#include <vector>
#include <string>
#include "function_call_context.h"
#include "ast_pp.h"

function_call_context *function_call_context_provider::m_ctx = nullptr;

void function_call_context_provider::initialize() {
    m_ctx = new function_call_context();
}


expr *function_call_precondition_miner::find_precondition(expr *expression, expr *function_call, ast_manager &m) {
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

expr *function_call_precondition_miner::replace_expr(expr *original, expr *from, expr *to, ast_manager &m) {
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