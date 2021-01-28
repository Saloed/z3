//
// Created by napst on 05.11.2020.
//

#include <ast/expr_functors.h>
#include <ast/rewriter/expr_replacer.h>
#include <unordered_map>
#include <functional>
#include "theory_function_call.h"
#include "smt/smt_context.h"
#include "smt_model_generator.h"

#include "ast/function_call_context.h"

namespace smt {

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

//        expr *last_arg = n->get_arg(num_args - 1);
//        enode *last_arg_e = ctx.get_enode(last_arg);
//        if (!is_attached_to_var(last_arg_e))
//            mk_var(last_arg_e);

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
        std::cout << "New eq" << std::endl;
    }


    void theory_function_call::new_diseq_eh(theory_var v1, theory_var v2) {
        std::cout << "New diseq" << std::endl;
    }

    void theory_function_call::relevant_eh(app *n) {
        if (!is_function_call(n)) return;
        std::cout << "relevant_eh" << "\n" << mk_pp(n, m) << std::endl;
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

    std::string clause_str(context &ctx, clause &cls) {
        vector<std::string> literals;
        for (unsigned i = 0; i < cls.get_num_literals(); ++i) {
            std::stringstream str_builder;
            ctx.display_literal_smt2(str_builder, cls.get_literal(i));
            literals.push_back(str_builder.str());
        }
        std::sort(literals.begin(), literals.end());

        std::stringstream str_builder;
        for (auto &&literal : literals) {
            str_builder << literal << "\n";
        }
        return str_builder.str();
    }

    void dump_clauses(unsigned log_index, std::string &file_name, vector<std::string> &new_aux,
                      vector<std::string> &del_aux) {
        std::ofstream aux_dump(file_name, std::ios_base::out | std::ios_base::app);

        aux_dump << "\nPropagate: " << log_index << "\n";
        aux_dump << "New:\n";
        for (auto &&aux: new_aux) {
            aux_dump << aux << "\n";
        }
        aux_dump << "Delete:\n";
        for (auto &&aux: del_aux) {
            aux_dump << aux << "\n";
        }
        aux_dump.close();
    }

    template<typename T>
    void ptr_vec_diff_printer(
            std::ostream &out,
            std::unordered_map<unsigned int, std::string> &history,
            ptr_vector<T> &vec,
            std::function<void(std::ostream &, T *, unsigned)> element_printer) {

        vector<std::string> added;
        vector<std::string> changed;

        for (unsigned i = 0; i < vec.size(); ++i) {
            T *e = vec[i];
            std::stringstream ss;
            element_printer(ss, e, i);
            std::string element_repr = ss.str();

            auto &&it = history.find(i);
            if (it == history.end()) {
                added.push_back(element_repr);
                history.emplace(i, element_repr);
            } else if (it->second != element_repr) {
                changed.push_back(element_repr);
                history.erase(i);
                history.emplace(i, element_repr);
            }
        }

        out << "New:\n";
        for (auto &it: added) {
            out << it << "\n";
        }
        out << "Changed:\n";
        for (auto &it: changed) {
            out << it << "\n";
        }
    }

    void print_expr(std::ostream &out, expr *e, unsigned i, context &ctx) {
        out << i << ": " << mk_pp(e, ctx.get_manager()) << "\n";
        b_justification js = ctx.m_bdata[i].justification();
        ctx.display_justification(out, js) << "\n";
    }


    void theory_function_call::propagate() {
        ptr_vector<clause> lemmas = ctx.get_lemmas();

        std::cout << "Propagate " << m_propagate_idx << std::endl;

        auto &&bv2Expr_printer = [&](std::ostream &out, expr *e, unsigned i) {
            return print_expr(out, e, i, ctx);
        };

        m_propagate_idx++;
        analyze_all_exprs_via_axiom();
        m_num_pending_queries = 0;
    }


    proof *get_proof_if_available(justification *js) {
        if (js == nullptr) return nullptr;
        if (strcmp(js->get_name(), "proof-wrapper") != 0) return nullptr;
        auto *pw = dynamic_cast<smt::justification_proof_wrapper *>(js);
        return pw->m_proof;
    }

    struct axiom_expr_info {
        ptr_vector<expr> exprs;
        unsigned arg_idx;
        theory_function_call::call_info call;
    };

    void theory_function_call::analyze_all_exprs_via_axiom() {

        vector<axiom_expr_info> relevant_exprs;
        for (auto &&call : registered_calls) {
            for (unsigned ai = 0; ai < call.out_args.size(); ai++) {
                ptr_vector<expr> to_analyze;
                for (unsigned bv = 0; bv < ctx.m_bool_var2expr.size(); ++bv) {
                    if (visited_expr.find(bv) != visited_expr.end()) {
                        continue;
                    }
                    visited_expr.emplace(bv);
                    expr *e = ctx.m_bool_var2expr[bv];
                    if (e == nullptr) continue;
                    to_analyze.push_back(e);
                }
                for (unsigned js_idx = 0; js_idx < ctx.m_justifications.size(); ++js_idx) {
                    justification *js = ctx.m_justifications[js_idx];
                    if (visited_js.find(js) != visited_js.end()) {
                        continue;
                    }
                    visited_js.emplace(js);
                    proof *js_proof = get_proof_if_available(js);
                    if (js_proof == nullptr) continue;
                    to_analyze.push_back(js_proof);
                }


                for (expr *e: to_analyze) {
                    ptr_vector<expr> sub = least_logical_subexpr_containing_expr(e, call.out_args[ai]);
                    axiom_expr_info info{sub, ai, call};
                    relevant_exprs.push_back(info);
                }
            }
        }

        for (auto &&info : relevant_exprs) {
            for (auto &&e: info.exprs) {
                auto &&precondition_ref = function_call_context_provider::get_context()->mk_call_axiom_for_expr(
                        e, info.call.call, m
                );
                if (!precondition_ref) continue;

                expr *precondition = precondition_ref.get();

                ctx.internalize(precondition, is_quantifier(precondition));
                ctx.mark_as_relevant(precondition);
                m.inc_ref(precondition);

                literal lit = mk_eq(e, precondition, is_quantifier(precondition));
                mk_th_axiom(lit);
            }
        }
    }

    expr *theory_function_call::replace_expr(expr *original, expr *from, expr *to) {
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

    ptr_vector<expr> theory_function_call::least_logical_subexpr_containing_expr(expr *e, expr *target) {
        ptr_vector<expr> result;
        llsce(m, e, target, nullptr, result, false);
        return result;
    }

    model_value_proc *theory_function_call::mk_value(enode *n, model_generator &mg) {
        std::cout << "Model for: " << enode_pp(n, ctx) << std::endl;
        return alloc(expr_wrapper_proc, n->get_expr());
    }

    expr_ref mk_implies_simplified(expr_ref &lhs, expr_ref &rhs) {
        auto &&m = lhs.get_manager();
        expr *implies = m.mk_or(m.mk_not(lhs), rhs);
        expr_ref result(implies, m);
        return result;
    }


    void theory_function_call::mk_th_axiom(literal &lit) {
        ctx.mark_as_relevant(lit);
        literal lits[1] = {lit};
        ctx.mk_th_axiom(get_id(), 1, lits);
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

}
