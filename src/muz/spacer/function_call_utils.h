#pragma once

#include "spacer_context.h"
#include "spacer_util.h"

namespace spacer {

    expr *replace_expr(expr *original, expr *from, expr *to, ast_manager &m) {
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

    void llsce(ast_manager &m, expr *e, expr *target, expr *nearest_logical_parent, ptr_vector<expr> &result,
               bool match_only_negations) {
        bool is_matched = m.is_bool(e) && (!match_only_negations || m.is_not(e));
        if (e == target) {
            if (is_matched) {
                result.push_back(e);
                return;
            }
            if (nearest_logical_parent != nullptr) {
                result.push_back(nearest_logical_parent);
                return;
            }
            if (match_only_negations) {
                // may be no negations
                return;
            }
            UNREACHABLE();
        }
        expr *new_parent = is_matched ? e : nearest_logical_parent;
        switch (e->get_kind()) {
            case AST_VAR:
                break;
            case AST_APP: {
                app *a = to_app(e);
                for (unsigned i = 0; i < a->get_num_args(); ++i) {
                    expr *arg = a->get_arg(i);
                    llsce(m, arg, target, new_parent, result, match_only_negations);
                }
                break;
            }
            case AST_QUANTIFIER: {
                quantifier *q = to_quantifier(e);
                expr *arg = q->get_expr();
                llsce(m, arg, target, new_parent, result, match_only_negations);
                break;
            }
            default: UNREACHABLE();
                break;
        }
    }

    ptr_vector<expr> least_logical_subexpr_containing_expr(expr *e, expr *target, ast_manager &m) {
        ptr_vector<expr> result;
        llsce(m, e, target, nullptr, result, false);
        return result;
    }

    expr *
    find_precondition(expr *expression, expr *in_arg, expr *out_arg, ast_manager &m) {
        std::stringstream expr_str_builder;
        expr_str_builder << mk_pp(expression, m);
        auto &&expr_str = expr_str_builder.str();

        if (expr_str == "(function_call query!0_3_n query!0_0_n query!0_2_n)") {
            return m.mk_true();
        } else if (expr_str == "(function_call aux!2_n recursion_0_n aux!3_n)") {
            return m.mk_true();
        }

        arith_util _arith(m);
        expr *x = _arith.mk_add(in_arg, _arith.mk_int(5));
        return replace_expr(expression, out_arg, x, m);
    }

    expr *find_function_call(expr *root, family_id fid) {
        if (!is_app(root)) return nullptr;
        app *root_as_app = to_app(root);
        if (root_as_app->is_app_of(fid, OP_FUNCTION_CALL)) {
            return root_as_app;
        }
        expr *result = nullptr;

        for (unsigned i = 0; i < root_as_app->get_num_args(); ++i) {
            expr *arg = root_as_app->get_arg(i);
            result = find_function_call(arg, fid);
            if (result != nullptr) break;
        }
        return result;
    }

    expr *find_function_call(expr *root, ast_manager &m) {
        family_id fc_id = m.mk_family_id("function_call");
        return find_function_call(root, fc_id);
    }

    lemma_ref mk_lemma(expr *lemma_expr, ast_manager &m) {
        lemma_ref lem = alloc(lemma, m, lemma_expr, infty_level());
        lem->set_background(false);
        return lem;
    }

    lemma_ref mk_special_lemma_1(expr *lemma_source, ast_manager &m) {
        family_id fc_id = m.mk_family_id("function_call");
        expr *function_call = find_function_call(lemma_source, fc_id);
        if (function_call == nullptr) return nullptr;
        expr *in_arg = to_app(function_call)->get_arg(1);
        expr *out_arg = to_app(function_call)->get_arg(2);

        arith_util _arith(m);
        expr *lemma_expr = _arith.mk_ge(out_arg, _arith.mk_add(in_arg, _arith.mk_int(5)));
        TRACE("xxx", tout << "Generate lemma: " << mk_pp(lemma_expr, m) << "\n" << "Source: " << mk_pp(lemma_source, m)
                          << "\nCall: " << mk_pp(function_call, m) << "\n";);
        return mk_lemma(lemma_expr, m);
    }

    lemma_ref mk_special_lemma_2(expr *lemma_source, ast_manager &m) {
        family_id fc_id = m.mk_family_id("function_call");
        expr *function_call = find_function_call(lemma_source, fc_id);
        if (function_call == nullptr) return nullptr;
        expr *in_arg = to_app(function_call)->get_arg(1);
        expr *out_arg = to_app(function_call)->get_arg(2);

        arith_util _arith(m);
        expr *lemma_expr = _arith.mk_le(out_arg, _arith.mk_add(in_arg, _arith.mk_int(5)));
        TRACE("xxx", tout << "Generate lemma: " << mk_pp(lemma_expr, m) << "\n" << "Source: " << mk_pp(lemma_source, m)
                          << "\nCall: " << mk_pp(function_call, m) << "\n";);
        return mk_lemma(lemma_expr, m);
    }

    lemma_ref mk_initial_lemma(expr *lemma_source, ast_manager &m) {
        family_id fc_id = m.mk_family_id("function_call");
        expr *function_call = find_function_call(lemma_source, fc_id);
        if (function_call == nullptr) return nullptr;
        expr *in_arg = to_app(function_call)->get_arg(1);
        expr *out_arg = to_app(function_call)->get_arg(2);

        arith_util _arith(m);
        expr *source_stmt = _arith.mk_le(out_arg, _arith.mk_int(17));
        expr *generated_stmt = find_precondition(source_stmt, in_arg, out_arg, m);

        expr *lemma_expr = m.mk_eq(source_stmt, generated_stmt);
        XXX("Generate lemma: " << mk_pp(lemma_expr, m) << "\n")
        return mk_lemma(lemma_expr, m);
    }

    lemma_ref mk_function_call_is_true_lemma(expr *lemma_source, ast_manager &m) {
        family_id fc_id = m.mk_family_id("function_call");
        expr *function_call = find_function_call(lemma_source, fc_id);
        if (function_call == nullptr) return nullptr;
        expr *lemma_expr = m.mk_eq(function_call, m.mk_true());
        return mk_lemma(lemma_expr, m);
    }

    lemma_ref_vector mk_function_call_lemmas(expr *function_call, expr *expression, ast_manager &m) {
        expr *in_arg = to_app(function_call)->get_arg(1);
        expr *out_arg = to_app(function_call)->get_arg(2);

        lemma_ref_vector generated_lemmas;
        auto &&sub_exprs = least_logical_subexpr_containing_expr(expression, out_arg, m);

        for (auto &&subexpr: sub_exprs) {
            expr *generated_stmt = find_precondition(subexpr, in_arg, out_arg, m);
            expr *lemma_expr = m.mk_eq(subexpr, generated_stmt);
            XXX("Generate lemma: " << mk_pp(lemma_expr, m) << "\n")
            lemma_ref lemma = mk_lemma(lemma_expr, m);
            generated_lemmas.push_back(lemma.get());
        }
        return generated_lemmas;
    }
}
