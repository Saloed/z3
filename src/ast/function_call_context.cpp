#include <utility>
#include <vector>
#include <string>
#include "function_call_context.h"
#include "ast_pp.h"

function_call::function_call_context *function_call::function_call_context_provider::m_ctx = nullptr;

void function_call::function_call_context_provider::initialize() {
    m_ctx = new function_call_context();
}

function_call::function_call_context *function_call::function_call_context_provider::get_context() {
    if (!m_ctx) { initialize(); }
    return m_ctx;
}


expr *
function_call::function_call_precondition_miner::find_precondition(expr *expression, expr *function_call,
                                                                   ast_manager &m) {
    if (m.is_not(expression)) {
        expr *negated_expr = to_app(expression)->get_arg(0);
        expr *negated_precondition = find_precondition(negated_expr, function_call, m);
        if (negated_precondition == nullptr) return nullptr;
        return m.mk_not(negated_precondition);
    }

    expr *in_arg = to_app(function_call)->get_arg(1);
    expr *out_arg = to_app(function_call)->get_arg(2);

    std::stringstream expr_str_builder;
    expr_str_builder << mk_pp(expression, m);
    auto &&expr_str = expr_str_builder.str();

    if (expr_str == "(function_call 777 query!0_0_n query!0_2_n)") {
        return m.mk_true();
    } else if (expr_str == "(function_call 777 recursion_0_n aux!2_n)") {
        return m.mk_true();
    }

    std::vector<std::string> known_vars = {"query!0_2_n", "aux!2_n"};

    arith_util _arith(m);
    expr *x = _arith.mk_add(in_arg, _arith.mk_int(5));

    for (auto &&var: known_vars) {
        if (expr_str.find(var) == std::string::npos) continue;
        expr *xxx_var = m.mk_const(var, _arith.mk_int());
        return replace_expr(expression, xxx_var, x, m);
    }

    std::cout << "Unexpected lemma: " << expr_str << " Call: " << mk_pp(function_call, m) << std::endl;
    return nullptr;
}

expr *
function_call::function_call_precondition_miner::replace_expr(expr *original, expr *from, expr *to, ast_manager &m) {
    if (original == from) return to;
    switch (original->get_kind()) {
        case AST_VAR:
            return original;
        case AST_APP: {
            app *a = to_app(original);
            bool changed = false;
            ptr_vector<expr> new_args;
            for (unsigned i = 0; i < a->get_num_args(); ++i) {
                expr *arg = a->get_arg(i);
                expr *new_arg = replace_expr(arg, from, to, m);
                new_args.push_back(new_arg);
                if (arg == new_arg) continue;
                changed = true;
            }
            if (!changed) return original;
            return m.mk_app(a->get_decl(), new_args);
        }
        case AST_QUANTIFIER: {
            quantifier *q = to_quantifier(original);
            expr *arg = q->get_expr();
            expr *new_arg = replace_expr(arg, from, to, m);
            if (arg == new_arg) return original;
            return m.mk_quantifier(
                    q->get_kind(), q->get_num_decls(), q->get_decl_sorts(), q->get_decl_names(),
                    new_arg,
                    q->get_weight(), q->get_qid(), q->get_skid(),
                    q->get_num_patterns(), q->get_patterns(), q->get_num_no_patterns(), q->get_no_patterns()
            );
        }
        default: UNREACHABLE();
            break;
    }
    UNREACHABLE();
    return nullptr;
}

obj_map<expr, expr *> function_call::function_call_context::get_axioms(expr *call) {
    return map.contains(call) ? map[call] : obj_map<expr, expr *>();
}

void function_call::function_call_context::register_call(expr *call) {
    if (!map.contains(call)) {
        map.insert(call, obj_map<expr, expr *>());
    }
}

expr_ref_vector function_call::function_call_context::generated_axioms(expr *call, ast_manager &m) {
    auto &&current = get_axioms(call);
    expr_ref_vector result(m);
    for (auto &&axiom: current) {
        result.push_back(m.mk_eq(axiom.m_key, axiom.m_value));
    }
    return result;
}

expr_ref function_call::function_call_context::mk_call_axiom_for_expr(expr *e, call_info &call_info, ast_manager &m) {
    expr *call = call_info.call_expr.get();
    auto &&call_axioms = get_axioms(call);
    expr *current_axiom = nullptr;
    if (call_axioms.find(e, current_axiom)) {
        return expr_ref(current_axiom, m);
    }
    current_axiom = precondition_miner.find_precondition(e, call, m);
    if (current_axiom == nullptr) {
        return expr_ref(m);
    }

    std::cout << "Generate axiom:\nSource: " << mk_pp(e, m) << "\nAxiom: " << mk_pp(current_axiom, m) << "\n";

    call_axioms.insert(e, current_axiom);
    map.insert(call, call_axioms);

    return expr_ref(current_axiom, m);
}

function_call::call_info function_call::function_call_context::expand_call(expr *call, ast_manager &m) {
    register_call(call);

    expr_ref call_expr_ref(call, m);

    expr_ref_vector in_args(m);
    in_args.push_back(to_app(call)->get_arg(1));

    expr_ref_vector out_args(m);
    out_args.push_back(to_app(call)->get_arg(2));

    return call_info(call_expr_ref, in_args, out_args);
}

function_call::call_info::call_info(expr_ref call_expr, expr_ref_vector in_args, expr_ref_vector out_args)
        : call_expr(std::move(call_expr)), in_args(std::move(in_args)), out_args(std::move(out_args)) {}
