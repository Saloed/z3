//
// Created by napst on 05.11.2020.
//

#pragma once

#include <ast/function_call_context.h>
#include "smt/smt_theory.h"
#include "ast/function_call_decl_plugin.h"
#include "util/map.h"
#include "smt_clause.h"

namespace smt {
    class theory_function_call : public theory {
    public:

        expr_ref_vector known_calls;
        u_map<bool> visited_expr;
        uint64_t last_analysed_state_hash;

        void force_run_analysis() { last_analysed_state_hash = 0; }

        uint64_t state_hash() const;

        bool can_analyse() const { return last_analysed_state_hash != state_hash(); };

        void state_analysed() { last_analysed_state_hash = state_hash(); }

        bool is_function_call(app const* n) const { return n->is_app_of(get_id(), OP_FUNCTION_CALL); }

        bool is_function_call(expr const* n) const { return is_app(n) && is_function_call(to_app(n)); }

        bool is_function_call(enode const* n) const { return is_function_call(n->get_owner()); }

        explicit theory_function_call(context& ctx);

        ~theory_function_call() override = default;

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

        bool build_models() const override;

        void mk_th_axiom(literal& lit);

        ptr_vector<expr> least_logical_subexpr_containing_expr(expr* e, expr* target);

        void analyze_all_exprs_via_axiom();

        bool is_analysed(expr* e) const;

        void mark_analysed(expr* e);

        void expr_precondition_axiom(expr* e, function_call::expanded_call& call);

        void analyse_expr_via_axiom(expr* e, function_call::expanded_call& call);
    };
}