#include <utility>
#include <vector>
#include <string>
#include "function_call_context.h"
#include "ast_pp.h"
#include "function_call_decl_plugin.h"


expr *
function_call::function_call_precondition_miner::find_precondition(expr *e, expr *call, unsigned call_id) {
    if (m.is_not(e)) {
        expr *negated_expr = to_app(e)->get_arg(0);
        expr *negated_precondition = find_precondition(negated_expr, call, 0);
        if (negated_precondition == nullptr) return nullptr;
        return m.mk_not(negated_precondition);
    }

    std::stringstream expr_str_builder;
    expr_str_builder << mk_pp(e, m);
    auto &&expr_str = expr_str_builder.str();

    if (call_id == 777) {
        expr *in_arg = to_app(call)->get_arg(1);
        expr *out_arg = to_app(call)->get_arg(2);

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
            return replace_expr(e, xxx_var, x);
        }
    } else if (call_id == 123) {
        if (expr_str == "(function_call 123 query!0_0_n query!0_1_n query!0_3_n)") {
            return m.mk_true();
        } else if (expr_str == "(function_call 123 recursion_0_n recursion_1_n aux!1_n)") {
            return m.mk_true();
        }
        std::vector<std::string> known_vars = {"query!0_3_n", "aux!1_n"};

        expr *in_idx = to_app(call)->get_arg(1);
        expr *in_arr = to_app(call)->get_arg(2);
        expr *out_arr = to_app(call)->get_arg(3);

        array_util _arr(m);
        arith_util _arith(m);

        expr_ref_vector in_val_args(m);
        in_val_args.push_back(in_arr);
        in_val_args.push_back(in_idx);
        expr *in_val = _arr.mk_select(in_val_args);
        expr *update_val = _arith.mk_add(in_val, _arith.mk_int(5));
        expr_ref_vector out_val_args(m);
        out_val_args.push_back(in_arr);
        out_val_args.push_back(in_idx);
        out_val_args.push_back(update_val);
        expr *x = _arr.mk_store(out_val_args); // (store in i (select in i) + 5)


        for (auto &&var: known_vars) {
            if (expr_str.find(var) == std::string::npos) continue;
            return replace_expr(e, out_arr, x);
        }

    }

    std::cout << "Unexpected lemma: " << expr_str << " Call: " << mk_pp(call, m) << std::endl;
    return nullptr;
}

expr *
function_call::function_call_precondition_miner::replace_expr(expr *original, expr *from, expr *to) {
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
                expr *new_arg = replace_expr(arg, from, to);
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
            expr *new_arg = replace_expr(arg, from, to);
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

unsigned parse_id(expr *id_expr, ast_manager &m) {
    // fixme: rewrite this
    std::stringstream ss;
    ss << mk_pp(id_expr, m);
    std::string id_str = ss.str();
    return std::stoi(id_str);
}

unsigned function_call::function_call_context::get_function_id(expr *call) {
    expr *id_expr = to_app(call)->get_arg(0);
    return parse_id(id_expr, m);
}

function_call::function_call_precondition_miner::function_call_precondition_miner(ast_manager &m) : m(m) {}

void function_call::function_call_context::register_call(expr *call) {
    axiom_storage.get_call_axioms(call);
}

expr_ref_vector function_call::function_call_context::generated_axioms(expr *call) {
    auto &&current = axiom_storage.get_call_axioms(call);
    expr_ref_vector result(m);
    for (auto &&it = current->begin(); it != current->end(); ++it) {
        result.push_back(m.mk_eq(it->m_key, it->m_value));
    }
    return result;
}

expr_ref function_call::function_call_context::mk_call_axiom_for_expr(expr *e, call_info &call_info) {
    expr *call = call_info.call_expr.get();
    auto &&call_axioms = axiom_storage.get_call_axioms(call);

    if (call_axioms->contains(e)) {
        expr *axiom;
        proof *p;
        call_axioms->get(e, axiom, p);
        return expr_ref(axiom, m);
    }

    expr *current_axiom = precondition_miner.find_precondition(e, call, get_function_id(call));
    if (current_axiom == nullptr) {
        return expr_ref(m);
    }

    XXX("Generate axiom:\nSource: " << mk_pp(e, m) << "\nAxiom: " << mk_pp(current_axiom, m) << "\n")

    call_axioms->insert(e, current_axiom, nullptr);
    return expr_ref(current_axiom, m);
}

void expand_call_args(unsigned call_id, expr *call, expr_ref_vector &in_args, expr_ref_vector &out_args) {
    if (call_id == 777) {
        in_args.push_back(to_app(call)->get_arg(1));
        out_args.push_back(to_app(call)->get_arg(2));
    } else if (call_id == 123) {
        in_args.push_back(to_app(call)->get_arg(1));
        in_args.push_back(to_app(call)->get_arg(2));
        out_args.push_back(to_app(call)->get_arg(3));
    } else {
        std::cout << "Expand: unexpected call id " << call_id << std::endl;
    }
}

function_call::call_info function_call::function_call_context::expand_call(expr *call) {
    register_call(call);

    expr_ref call_expr_ref(call, m);
    expr_ref_vector in_args(m);
    expr_ref_vector out_args(m);

    expand_call_args(get_function_id(call), call, in_args, out_args);

    return call_info(call_expr_ref, in_args, out_args);
}

function_call::function_call_context::function_call_context(ast_manager &m)
        : m(m), precondition_miner(m), axiom_storage(m) {
    function_call_family_id = m.mk_family_id("function_call");
}

expr *function_call::function_call_context::find_function_call(expr *root) {
    if (!is_app(root)) return nullptr;
    app *root_as_app = to_app(root);
    if (root_as_app->is_app_of(function_call_family_id, OP_FUNCTION_CALL)) {
        return root_as_app;
    }
    expr *result = nullptr;

    for (unsigned i = 0; i < root_as_app->get_num_args(); ++i) {
        expr *arg = root_as_app->get_arg(i);
        result = find_function_call(arg);
        if (result != nullptr) break;
    }
    return result;
}

void function_call::function_call_context::extend_forms_with_generated_axioms(expr_ref_vector &forms) {
    for (auto &&form: forms) {
        extend_forms_with_generated_axioms(form, forms);
    }
}

void function_call::function_call_context::extend_forms_with_generated_axioms(expr *form, expr_ref_vector &forms) {
    expr *function_call = find_function_call(form);
    if (function_call == nullptr) return;
    auto &&registered = generated_axioms(function_call);
    for (auto &&e: registered) {
        forms.push_back(e);
    }
}

void function_call::function_call_context::update_function_call_analyzer(api::function_call_analyzer *analyzer) {
    analyzer->find_precondition(nullptr, 0, nullptr, 0, nullptr, 0); // todo
}

function_call::call_info::call_info(expr_ref call_expr, expr_ref_vector in_args, expr_ref_vector out_args)
        : call_expr(std::move(call_expr)), in_args(std::move(in_args)), out_args(std::move(out_args)) {}

function_call::call_axiom_storage::call_axiom_storage(ast_manager &m) : m(m) {
}

expr_map *function_call::call_axiom_storage::get_call_axioms(expr *call) {
    unsigned call_id = call->get_id();
    unsigned call_axiom_idx;
    if (calls.find(call_id, call_axiom_idx)) {
        return axioms[call_axiom_idx];
    }
    call_axiom_idx = axioms.size();
    expr_map *axiom_map = alloc(expr_map, m, false);
    axioms.push_back(axiom_map);
    calls.insert(call_id, call_axiom_idx);
    return axioms[call_axiom_idx];
}

function_call::call_axiom_storage::~call_axiom_storage() {
    for (unsigned i = 0; i < axioms.size(); ++i) {
        expr_map *axiom_map = axioms[i];
        dealloc(axiom_map);
    }
}
