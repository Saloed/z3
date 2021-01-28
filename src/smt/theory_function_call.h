//
// Created by napst on 05.11.2020.
//

#pragma once

#include <unordered_set>
#include <unordered_map>
#include "smt/smt_theory.h"
#include "ast/function_call_decl_plugin.h"
#include "util/union_find.h"
#include "smt_clause.h"

namespace smt {
    class theory_function_call : public theory {
    public:

        ptr_vector<expr> known_calls;
        unsigned m_num_pending_queries;
        unsigned m_propagate_idx = 0;
        std::unordered_set<unsigned > visited_expr;
        std::unordered_set<justification *> visited_js;

        bool is_function_call(app const *n) const { return n->is_app_of(get_id(), OP_FUNCTION_CALL); }

        bool is_function_call(expr const *n) const { return is_app(n) && is_function_call(to_app(n)); }

        bool is_function_call(enode const *n) const { return is_function_call(n->get_owner()); }

        explicit theory_function_call(context &ctx);

        ~theory_function_call() override = default;

        void display_var(std::ostream &out, theory_var v) const;

        void display(std::ostream &out) const override;

        theory *mk_fresh(context *new_ctx) override {
            return alloc(theory_function_call, *new_ctx);
        }

        const char *get_name() const override { return "function-call"; }

        bool internalize_atom(app *atom, bool gate_ctx) override;

        bool internalize_term(app *term) override;

        void new_eq_eh(theory_var v1, theory_var v2) override;

        void new_diseq_eh(theory_var v1, theory_var v2) override;

        void relevant_eh(app *n) override;

        void init_search_eh() override;

        final_check_status final_check_eh() override;

        bool can_propagate() override;

        void propagate() override;

        bool is_shared(theory_var v) const override;

        bool internalize_term_core(app *n);

        bool build_models() const override;

        theory_var mk_var(enode *n) override;

        model_value_proc *mk_value(enode *n, model_generator &mg) override;

        void mk_th_axiom(literal& lit);

        ptr_vector<expr> least_logical_subexpr_containing_expr(expr *e, expr *target);

        void analyze_all_exprs_via_axiom();

    };
}