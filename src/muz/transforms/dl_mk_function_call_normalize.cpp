//
// Created by napst on 09.02.2021.
//

#include "ast/for_each_expr.h"
#include "muz/transforms/dl_mk_function_call_normalize.h"
#include "ast/ast_util.h"
#include "ast/function_call_context.h"

namespace datalog {
    mk_function_call_normalize::mk_function_call_normalize(context& ctx, unsigned int priority) :
            rule_transformer::plugin(priority, false),
            m_ctx(ctx),
            m(ctx.get_manager()),
            rm(ctx.get_rule_manager()) {}

    mk_function_call_normalize::~mk_function_call_normalize() {}

    rule_set* mk_function_call_normalize::operator()(const rule_set& source) {
        if (!m.enabled_function_call_support())
            return nullptr;

        scoped_ptr<rule_set> rules = alloc(rule_set, m_ctx);
        rules->inherit_predicates(source);
        bool change = false;
        for (auto* rule : source) {
            if (m_ctx.canceled()) {
                return nullptr;
            }
            change |= normalize(*rule, *rules);
        }
        if (!change) {
            rules = nullptr;
        }
        return rules.detach();
    }


    template<typename VAR_GEN>
    expr* mk_function_call_normalize::normalize_function_calls(
            expr* expression, VAR_GEN&& var_generator,
            function_call::function_call_context* f_ctx
    ) {
        switch (expression->get_kind()) {
            case AST_VAR:
                return expression;
            case AST_APP: {
                app* a = to_app(expression);
                if (f_ctx->is_function_call(a)) {
                    return normalize_function_call(a, var_generator);
                }
                bool changed = false;
                ptr_vector<expr> new_args;
                for (unsigned i = 0; i < a->get_num_args(); ++i) {
                    expr* arg = a->get_arg(i);
                    expr* new_arg = normalize_function_calls(arg, var_generator, f_ctx);
                    new_args.push_back(new_arg);
                    changed |= arg == new_arg;
                }
                if (!changed) return expression;
                return m.mk_app(a->get_decl(), new_args);
            }
            case AST_QUANTIFIER: {
                quantifier* q = to_quantifier(expression);
                expr* arg = q->get_expr();
                expr* new_arg = normalize_function_calls(arg, var_generator, f_ctx);
                if (arg == new_arg) return expression;
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

    template<typename VAR_GEN>
    expr* mk_function_call_normalize::normalize_function_call(app* function_call, VAR_GEN&& var_generator) {
        ptr_vector<expr> new_args, bindings;
        new_args.push_back(function_call->get_arg(0)); // function id
        for (unsigned i = 1; i < function_call->get_num_args(); ++i) {
            expr* arg = function_call->get_arg(i);
            if (is_var(arg)) {
                new_args.push_back(arg);
                continue;
            }
            expr* placeholder = var_generator(m.get_sort(arg));
            expr* binding = m.mk_eq(placeholder, arg);
            new_args.push_back(placeholder);
            bindings.push_back(binding);
        }
        if (bindings.empty()) {
            return function_call;
        }
        app* new_function_call = m.mk_app(function_call->get_decl(), new_args.size(), new_args.c_ptr());
        bindings.push_back(new_function_call);
        return m.mk_and(bindings);
    }

    bool mk_function_call_normalize::normalize(rule& r, rule_set& new_rules) {
        auto&& function_call_ctx = m.get_function_call_context();

        unsigned utsz = r.get_uninterpreted_tail_size();
        unsigned tsz = r.get_tail_size();
        expr_ref_vector new_conjs(m), normalized_conjs(m);

        for (unsigned i = utsz; i < tsz; ++i)
            new_conjs.push_back(r.get_tail(i));
        flatten_and(new_conjs);

        used_vars rule_vars;
        r.get_used_vars(rule_vars);
        unsigned bindings_var_idx = rule_vars.get_max_found_var_idx_plus_1();
        ast_manager& m_manager = m;
        auto binding_var_generator = [&bindings_var_idx, &m_manager](sort* var_sort) {
            unsigned var_idx = bindings_var_idx++;
            return m_manager.mk_var(var_idx, var_sort);
        };

        bool changed = false;
        for (auto&& conj: new_conjs) {
            expr* conj_function_call = function_call_ctx->find_function_call(conj);
            if (conj_function_call == nullptr) {
                normalized_conjs.push_back(conj);
                continue;
            }
            expr* normalized_conj = normalize_function_calls(conj, binding_var_generator, function_call_ctx);
            if (normalized_conj != conj) {
                changed = true;
            }
            normalized_conjs.push_back(normalized_conj);
        }

        if (!changed) {
            new_rules.add_rule(&r);
            return false;
        }

        app_ref_vector tail(m);
        for (unsigned i = 0; i < utsz; ++i) {
            tail.push_back(r.get_tail(i));
        }
        for (expr* e : normalized_conjs) {
            tail.push_back(rm.ensure_app(e));
        }
        rule_ref new_rule(rm.mk(r.get_head(), tail.size(), tail.c_ptr(), nullptr, r.name(), false), rm);
        rm.mk_rule_rewrite_proof(r, *new_rule.get());
        new_rules.add_rule(new_rule);
        TRACE("dl", tout << "Function calls normalized\n";);
        return true;
    }

}
