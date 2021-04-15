//
// Created by napst on 05.11.2020.
//

#include <ast/expr_functors.h>
#include <ast/rewriter/expr_replacer.h>
#include "smt/smt_context.h"
#include "smt_model_generator.h"
#include "theory_function_call.h"

namespace smt {

    theory_function_call::theory_function_call(context& ctx) :
            theory(ctx, ctx.get_manager().mk_family_id("function_call")),
            last_analysed_state_hash(0), known_calls(m) {
    }

    void theory_function_call::display(std::ostream &out) const {
        out << "Theory function call:\n" << known_calls << "\n";
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

        known_calls.push_back(n);

        if (m.is_bool(n)) {
            bool_var bv = ctx.mk_bool_var(n);
            ctx.set_var_theory(bv, get_id());
            ctx.set_enode_flag(bv, true);
        }
        return true;
    }

    theory_var theory_function_call::mk_var(enode *n) {
        theory_var r = theory::mk_var(n);
        ctx.attach_th_var(n, this, r);
        return r;
    }

    bool theory_function_call::internalize_term(app *n) {
        TRACE("xxx", tout << "Internalize: " << mk_pp(n, m) << "\n";);
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
    }


    void theory_function_call::new_diseq_eh(theory_var v1, theory_var v2) {
    }

    void theory_function_call::relevant_eh(app *n) {
        if (!is_function_call(n)) return;
        force_run_analysis();
        if (!ctx.e_internalized(n)) ctx.internalize(n, false);
    }

    void theory_function_call::init_search_eh() {
        force_run_analysis();
    }

    final_check_status theory_function_call::final_check_eh() {
        if (!can_analyse()) return FC_DONE;
        return FC_CONTINUE;
    }

    bool theory_function_call::is_shared(theory_var v) const {
        return false;
    }

    bool theory_function_call::can_propagate() {
        return can_analyse();
    }

    void theory_function_call::propagate() {
        state_analysed();
        analyze_all_exprs_via_axiom();
    }

    void theory_function_call::analyze_all_exprs_via_axiom() {
        vector<function_call::expanded_call> registered_calls;
        for (auto&& call: known_calls) {
            auto&& expanded_call = m.get_function_call_context()->expand_call(call);
            registered_calls.push_back(expanded_call);
        }

        ptr_vector<expr> exprs_to_analyze;
        for (expr* e : ctx.m_bool_var2expr) {
            if (!e || is_analysed(e)) continue;
            exprs_to_analyze.push_back(e);
        }

        for (expr* e : exprs_to_analyze) {
            for (auto&& call : registered_calls) {
                analyse_expr_via_axiom(e, call);
            }
        }
    }

    bool theory_function_call::is_analysed(expr* e) const {
        return visited_expr.contains(e->get_id());
    }

    void theory_function_call::mark_analysed(expr* e) {
        visited_expr.insert(e->get_id(), true);
    }

    void theory_function_call::analyse_expr_via_axiom(expr* e, function_call::expanded_call& call) {
        for (unsigned ai = 0; ai < call.out_args.size(); ai++) {
            expr* out_arg = call.out_args[ai].get();
            ptr_vector<expr> sub_exps = least_logical_subexpr_containing_expr(e, out_arg);
            for (expr* sub_expr: sub_exps) {
                expr_precondition_axiom(sub_expr, call);
            }
        }
    }

    void theory_function_call::expr_precondition_axiom(expr* e, function_call::expanded_call& call) {
        if (is_analysed(e)) return;

        auto&& precondition_ref = m.get_function_call_context()->mk_call_axiom_for_expr(e, call);
        if (!precondition_ref) return;
        expr* precondition = precondition_ref.get();

        mark_analysed(e);
        mark_analysed(precondition);

        ctx.internalize(precondition, is_quantifier(precondition));
        ctx.mark_as_relevant(precondition);

        literal lit = mk_eq(e, precondition, is_quantifier(precondition));
        mk_th_axiom(lit);
    }

    ptr_vector<expr> theory_function_call::least_logical_subexpr_containing_expr(expr* expression, expr* target) {
        ptr_vector<expr> result;
        svector<std::pair<expr*, expr*>> todo;
        todo.push_back(std::make_pair(expression, nullptr));
        while (!todo.empty()) {
            auto&& item = todo.back();
            expr* e = item.first;
            expr* nearest_logical_parent = item.second;
            todo.pop_back();

            bool is_logical = m.is_bool(e);
            if (e == target) {
                if (is_logical) {
                    result.push_back(e);
                    continue;
                }
                if (nearest_logical_parent != nullptr) {
                    result.push_back(nearest_logical_parent);
                    continue;
                }
                UNREACHABLE();
            }
            expr* new_parent = is_logical ? e : nearest_logical_parent;
            switch (e->get_kind()) {
                case AST_VAR:
                    break;
                case AST_APP: {
                    app* a = to_app(e);
                    for (unsigned i = 0; i < a->get_num_args(); ++i) {
                        todo.push_back(std::make_pair(a->get_arg(i), new_parent));
                    }
                    break;
                }
                case AST_QUANTIFIER: {
                    todo.push_back(std::make_pair(to_quantifier(e)->get_expr(), new_parent));
                    break;
                }
                default: UNREACHABLE();
                    break;
            }
        }
        return result;
    }

    model_value_proc* theory_function_call::mk_value(enode* n, model_generator& mg) {
        std::cout << "Model for: " << enode_pp(n, ctx) << std::endl;
        return alloc(expr_wrapper_proc, n->get_expr());
    }

    expr_ref mk_implies_simplified(expr_ref& lhs, expr_ref& rhs) {
        auto&& m = lhs.get_manager();
        expr* implies = m.mk_or(m.mk_not(lhs), rhs);
        expr_ref result(implies, m);
        return result;
    }

    void theory_function_call::mk_th_axiom(literal& lit) {
        ctx.mark_as_relevant(lit);
        literal lits[1] = {lit};
        ctx.mk_th_axiom(get_id(), 1, lits);
        XXX("Add function axiom:\n"; ctx.display_literal_smt2(tout, lit); tout << "\n")
    }

    bool theory_function_call::build_models() const {
        return false;
    }

    inline uint64_t hash_combine(uint64_t seed, uint64_t value) {
        // boost hash_combine
        return seed ^ value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    inline uint64_t expr_hash(expr* e) {
        return reinterpret_cast<uint64_t>(e);
    }

    uint64_t hash_expr_vector(expr* const* vec, unsigned int size) {
        uint64_t hash = 17;
        for (unsigned i = 0; i < size; i++) {
            uint64_t el_hash = expr_hash(vec[i]);
            hash = hash_combine(hash, el_hash);
        }
        return hash;
    }

    uint64_t theory_function_call::state_hash() const {
        uint64_t bv2expr_hash = hash_expr_vector(ctx.m_bool_var2expr.c_ptr(), ctx.m_bool_var2expr.size());
        uint64_t calls_hash = hash_expr_vector(known_calls.c_ptr(), known_calls.size());
        return hash_combine(calls_hash, bv2expr_hash);
    }

}
