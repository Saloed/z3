//
// Created by napst on 05.11.2020.
//

#pragma once

#include "smt/smt_theory.h"
#include "ast/function_call_decl_plugin.h"
#include "util/union_find.h"
#include "smt_clause.h"

namespace smt {
    class theory_function_call : public theory {
    public:
        typedef trail_stack<theory_function_call> th_trail_stack;
        typedef union_find<theory_function_call> th_union_find;
        th_union_find m_find;
        th_trail_stack m_trail_stack;

        struct call_info {
            ptr_vector<expr> out_args;
            ptr_vector<expr> in_args;
            expr *call;

            call_info() : call(nullptr) {}
        };

        vector<call_info> registered_calls;
        unsigned m_num_pending_queries;

        th_trail_stack &get_trail_stack() { return m_trail_stack; }

        bool is_function_call_sort(sort const *s) const { return s->is_sort_of(get_id(), FUNCTION_CALL_SORT); }

        bool is_function_call_sort(app const *n) const { return is_function_call_sort(get_manager().get_sort(n)); }

        bool is_function_call_sort(enode const *n) const { return is_function_call_sort(n->get_owner()); }

        bool is_function_call(app const *n) const { return n->is_app_of(get_id(), OP_FUNCTION_CALL); }

        bool is_function_call(enode const *n) const { return is_function_call(n->get_owner()); }

        theory_function_call(context &ctx);

        ~theory_function_call() override = default;

        void display_var(std::ostream &out, theory_var v) const;

        void display(std::ostream &out) const override;

        theory *mk_fresh(context *new_ctx) override {
            return alloc(theory_function_call, *new_ctx);
        }

        theory_var find(theory_var v) const { return m_find.find(v); }

        bool internalize_atom(app *atom, bool gate_ctx) override;

        bool internalize_term(app *term) override;

        void new_eq_eh(theory_var v1, theory_var v2) override;

        void new_diseq_eh(theory_var v1, theory_var v2) override;

        void relevant_eh(app *n) override;

        void push_scope_eh() override;

        void init_search_eh() override;

        final_check_status final_check_eh() override;

        void pop_scope_eh(unsigned int num_scopes) override;

        bool can_propagate() override;

        void propagate() override;

        bool is_shared(theory_var v) const override;

        void restart_eh() override;

        bool internalize_term_core(app *n);

        void unmerge_eh(theory_var v1, theory_var v2);

        void merge_eh(theory_var v1, theory_var v2, theory_var, theory_var);

        void after_merge_eh(theory_var r1, theory_var r2, theory_var v1, theory_var v2);

        bool build_models() const override;

        theory_var mk_var(enode *n) override;

        void analyze_lemma(expr_ref &lemma, call_info &call, expr *argument);
    };
}