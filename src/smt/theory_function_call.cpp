//
// Created by napst on 05.11.2020.
//

#include <ast/expr_functors.h>
#include "theory_function_call.h"
#include "smt/smt_context.h"

namespace smt {


#define PP_EXPR(expr) DEBUG_CODE(smt2_pp_environment_dbg env(m); ast_smt2_pp(std::cout, expr, env); std::cout << std::endl;)


    theory_function_call::theory_function_call(context &ctx) :
            theory(ctx, ctx.get_manager().mk_family_id("function_call")),
            m_find(*this),
            m_trail_stack(*this),
            m_num_pending_queries(0) {
    }

    void theory_function_call::display(std::ostream &out) const {
        unsigned num_vars = get_num_vars();
        if (num_vars == 0) return;
        out << "Theory function call:\n";
        for (unsigned v = 0; v < num_vars; v++) {
            display_var(out, v);
        }
    }

    void theory_function_call::display_var(std::ostream &out, theory_var v) const {
        out << "theory var " << v << "\n";
    }


    bool theory_function_call::internalize_atom(app *atom, bool gate_ctx) {
        return internalize_term(atom);
    }

    bool theory_function_call::internalize_term_core(app *n) {
        unsigned num_args = n->get_num_args();
        for (unsigned i = 0; i < num_args; i++)
            ctx.internalize(n->get_arg(i), false);
        if (ctx.e_internalized(n)) {
            return false;
        }
        enode *e = ctx.mk_enode(n, false, false, true);
        if (!is_attached_to_var(e))
            mk_var(e);

        expr *last_arg = n->get_arg(num_args - 1);
        enode *last_arg_e = ctx.get_enode(last_arg);
        if (!is_attached_to_var(last_arg_e))
            mk_var(last_arg_e);

        call_info call;
        call.call = n;
        call.in_args.push_back(n->get_arg(1));
        call.out_args.push_back(n->get_arg(2));
        registered_calls.push_back(call);

        if (m.is_bool(n)) {
            bool_var bv = ctx.mk_bool_var(n);
            ctx.set_var_theory(bv, get_id());
            ctx.set_enode_flag(bv, true);
        }
        return true;
    }

    theory_var theory_function_call::mk_var(enode *n) {
        theory_var r = theory::mk_var(n);
        VERIFY(r == static_cast<theory_var>(m_find.mk_var()));
        ctx.attach_th_var(n, this, r);
        return r;
    }

    bool theory_function_call::internalize_term(app *n) {
        if (ctx.e_internalized(n)) {
            return true;
        }

        if (!internalize_term_core(n)) {
            return true;
        }
        enode *arg0 = ctx.get_enode(n->get_arg(0));
        if (!is_attached_to_var(arg0))
            mk_var(arg0);

        theory_var v_arg = arg0->get_th_var(get_id());

        SASSERT(v_arg != null_theory_var);
        return true;
    }


    void theory_function_call::new_eq_eh(theory_var v1, theory_var v2) {
        std::cout << "New eq" << std::endl;
    }


    void theory_function_call::new_diseq_eh(theory_var v1, theory_var v2) {
        std::cout << "New diseq" << std::endl;
    }

    void theory_function_call::relevant_eh(app *n) {
        if (!is_function_call(n)) return;
        std::cout << "relevant_eh" << std::endl;
        PP_EXPR(n);
        m_num_pending_queries++;
        if (!ctx.e_internalized(n)) ctx.internalize(n, false);
    }

    void theory_function_call::push_scope_eh() {
        theory::push_scope_eh();
        m_trail_stack.push_scope();
    }

    void theory_function_call::pop_scope_eh(unsigned int num_scopes) {
        m_trail_stack.pop_scope(num_scopes);
        theory::pop_scope_eh(num_scopes);
        SASSERT(m_find.get_num_vars() == get_num_vars());
    }

    void theory_function_call::init_search_eh() {
        m_num_pending_queries = 0;
    }

    void theory_function_call::restart_eh() {
        theory::restart_eh();
    }

    final_check_status theory_function_call::final_check_eh() {
        if (m_num_pending_queries == 0) return FC_DONE;
        return FC_CONTINUE;
    }

    bool theory_function_call::is_shared(theory_var v) const {
        return false;
    }

    bool theory_function_call::can_propagate() {
        return m_num_pending_queries != 0;
    }

    void theory_function_call::propagate() {
        std::cout << "Propagate" << std::endl;

        ptr_vector<clause> lemmas = ctx.get_lemmas();
        vector<expr_ref> lemmas_to_analyze;
        vector<call_info> calls_to_analyze;
        ptr_vector<expr> arg_to_analyze;

        for (auto &&call : registered_calls) {
            for (expr *out_arg : call.out_args) {
                contains_expr arg_matcher(m, out_arg);
                for (auto &&lemma : lemmas) {
                    for (unsigned i = 0; i < lemma->get_num_literals(); i++) {
                        literal lemma_literal = lemma->get_literal(i);
                        expr_ref literal_expr = ctx.literal2expr(lemma_literal);
                        if (arg_matcher(literal_expr)) {
                            lemmas_to_analyze.push_back(literal_expr);
                            calls_to_analyze.push_back(call);
                            arg_to_analyze.push_back(out_arg);
                        }
                    }
                }
            }
        }


        for (auto &&expr: lemmas_to_analyze) {
            PP_EXPR(expr);
        }

        for (unsigned i = 0; i < lemmas_to_analyze.size(); ++i) {
            analyze_lemma(lemmas_to_analyze[i], calls_to_analyze[i], arg_to_analyze[i]);
        }

        m_num_pending_queries = 0;
    }

    void theory_function_call::merge_eh(theory_var v1, theory_var v2, theory_var, theory_var) {
        SASSERT(v1 == find(v1));
    }

    void theory_function_call::after_merge_eh(theory_var r1, theory_var r2, theory_var v1, theory_var v2) {
        // do nothing
    }

    void theory_function_call::unmerge_eh(theory_var v1, theory_var v2) {
        // do nothing
    }

    bool theory_function_call::build_models() const {
        return false;
    }

    void theory_function_call::analyze_lemma(expr_ref &lemma, theory_function_call::call_info &call, expr *argument) {

    }


}
