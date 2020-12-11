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

    void
    log_clauses(context &ctx, ptr_vector<clause> &clauses, std::unordered_set<std::string> &aux_history,
                unsigned log_index, std::string file_name) {
        vector<std::string> current_aux;
        std::unordered_set<std::string> current_aux_set;
        for (auto &&aux: clauses) {
            auto &&aux_str = clause_str(ctx, *aux);
            current_aux.push_back(aux_str);
            current_aux_set.emplace(aux_str);
        }

        vector<std::string> new_aux;
        for (auto &&aux: current_aux) {
            if (aux_history.find(aux) != aux_history.end()) continue;
            new_aux.push_back(aux);
        }

        vector<std::string> delete_aux;
        for (auto &&aux: aux_history) {
            if (current_aux_set.find(aux) != current_aux_set.end()) continue;
            delete_aux.push_back(aux);
        }

        aux_history.clear();
        for (auto &&aux: current_aux) {
            aux_history.emplace(aux);
        }

        dump_clauses(log_index, file_name, new_aux, delete_aux);
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

    void print_bv2expr(std::ostream &out, ptr_vector<expr> &bv2e, context &ctx) {
        for (unsigned i = 0; i < bv2e.size(); ++i) {
            expr *e = bv2e[i];
            print_expr(out, e, i, ctx);
        }
    }

    void display_justifications(std::ostream &out, context &ctx) {
        for (unsigned i = 0; i < ctx.m_justifications.size(); i++) {
            out << "Justification: " << i << "\n";
            ctx.display_justification(out, ctx.m_justifications[i]);
            out << "\n";
        }
    }

    void theory_function_call::propagate() {
        ptr_vector<clause> lemmas = ctx.get_lemmas();

//        std::ofstream bv_trace(".z3_bv2expr", std::ios_base::out | std::ios_base::app);
//        std::ofstream w_trace(".z3_watches", std::ios_base::out | std::ios_base::app);
//        bv_trace << "Propagate " << m_propagate_idx << std::endl;
//        w_trace << "Propagate " << m_propagate_idx << std::endl;
//        print_bv2expr(bv_trace, ctx.m_bool_var2expr, m, ctx);
//        ctx.display_watches_smt2(w_trace);
//        log_clauses(ctx, ctx.m_aux_clauses, aux_history, m_propagate_idx, ".z3-aux_trace");
//        log_clauses(ctx, lemmas, lemma_history, m_propagate_idx, ".z3-lemma_trace");

        std::cout << "Propagate " << m_propagate_idx << std::endl;
        TRACE("xxx", tout << "Propagate: " << m_propagate_idx << "\n";);
//        TRACE("xxx", tout << "Justifications:\n"; display_justifications(tout, ctx););
//        TRACE("xxx", tout << "watches on start" << std::endl; ctx.display_watches_smt2(tout););

        auto &&bv2Expr_printer = [&](std::ostream &out, expr *e, unsigned i) {
            return print_expr(out, e, i, ctx);
        };
//        TRACE("xxx", ptr_vec_diff_printer<expr>(tout, bv2Expr_history, ctx.m_bool_var2expr, bv2Expr_printer););

        //        TRACE("xxx", print_bv2expr(tout, ctx.m_bool_var2expr,  ctx););

//        ctx.m_conflict_resolution->reset();

        if (m_propagate_idx == 5) {
//            std::cout << "bv2e:\n";
//            ctx.display_bool_var_defs(std::cout);
//            std::cout << "\nEnodes:\n";
//            ctx.display_enode_defs(std::cout);
//            std::cout << "asserted formulas:\n";
//            ctx.display_asserted_formulas(std::cout);
//            std::cout << "\nbinary clauses:\n";
//            ctx.display_binary_clauses(std::cout);
//            std::cout << "\nauxiliary clauses:\n";
//            ctx.display_clauses(std::cout, ctx.m_aux_clauses);
//            std::cout << "\nlemmas:\n";
//            ctx.display_clauses(std::cout, ctx.m_lemmas);
//            std::cout << "\nassignment:\n";
//            ctx.display_assignment(std::cout);
//            std::cout << "\neqc:\n";
//            ctx.display_eqc(std::cout);
//            std::cout << "\ncg table:\n";
//            ctx.m_cg_table.display_compact(std::cout);
//            std::cout << "\nxase split queue:\n";
//            ctx.m_case_split_queue->display(std::cout);
//            std::cout << "\nbool var map:\n";
//            ctx.display_expr_bool_var_map(std::cout);
//            std::cout << "\nenode map:\n";
//            ctx.display_app_enode_map(std::cout);
//            std::cout << "\nrelevant exprs:\n";
//            ctx.display_relevant_exprs(std::cout);
//            ctx.display(std::cout);
            std::cout << std::flush;
        }
        m_propagate_idx++;

//        std::ofstream ctx_trace("z3_ctx_trace.txt", std::ios_base::out | std::ios_base::app);
//        ctx.display(ctx_trace);
//        ctx_trace.close();


//        analyze_all_exprs_via_replace();
        analyze_all_exprs_via_axiom();

        m_num_pending_queries = 0;
    }


    proof *get_proof_if_available(justification *js) {
        if (js == nullptr) return nullptr;
        if (strcmp(js->get_name(), "proof-wrapper") != 0) return nullptr;
        auto *pw = dynamic_cast<smt::justification_proof_wrapper *>(js);
        return pw->m_proof;
    }

    enum replace_expr_info_type {
        BV_EXPR, JUSTIFICATION
    };

    struct replace_expr_info {
        replace_expr_info_type type;
        unsigned idx;
        unsigned arg_idx;
        ptr_vector<expr> relevant_expr;
        theory_function_call::call_info call;
    };

    void theory_function_call::analyze_all_exprs_via_replace() {
        ctx.m_conflict_resolution->reset();

        vector<replace_expr_info> relevant_exprs;
        for (auto &&call : registered_calls) {
            for (unsigned ai = 0; ai < call.out_args.size(); ai++) {
                for (unsigned bv = 0; bv < ctx.m_bool_var2expr.size(); ++bv) {
                    expr *e = ctx.m_bool_var2expr[bv];
                    if (e == nullptr) continue;
                    ptr_vector<expr> sub = least_logical_subexpr_containing_expr(e, call.out_args[ai]);
                    replace_expr_info info{BV_EXPR, bv, ai, sub, call};
                    relevant_exprs.push_back(info);
                }
                for (unsigned js_idx = 0; js_idx < ctx.m_justifications.size(); ++js_idx) {
                    justification *js = ctx.m_justifications[js_idx];
                    proof *js_proof = get_proof_if_available(js);
                    if (js_proof == nullptr) continue;
                    ptr_vector<expr> sub = least_logical_subexpr_containing_expr(js_proof, call.out_args[ai]);
                    replace_expr_info info{JUSTIFICATION, js_idx, ai, sub, call};
                    relevant_exprs.push_back(info);
                }
            }
        }

        for (auto &&info : relevant_exprs) {
            if (info.type == BV_EXPR) {
                if (visited_expr.find(info.idx) != visited_expr.end()) {
                    continue;
                }
                visited_expr.emplace(info.idx);

                expr *original_expr = ctx.bool_var2expr(info.idx);

                bool has_precondition = false;
                expr *replacement = original_expr;
                for (auto *sub: info.relevant_expr) {
                    expr *precondition = find_precondition(sub, info.call, info.call.out_args[info.arg_idx]);
                    if (precondition == nullptr) continue;
                    has_precondition = true;
                    replacement = replace_expr(replacement, sub, precondition);
                }
                if (!has_precondition) continue;

                m.inc_ref(replacement);
                ctx.internalize(replacement, is_quantifier(replacement));

                ctx.m_bool_var2expr[info.idx] = replacement;
            }

            if (info.type == JUSTIFICATION) {
                justification *js = ctx.m_justifications[info.idx];
                if (visited_js.find(js) != visited_js.end()) {
                    continue;
                }
                visited_js.emplace(js);
                proof *original_expr = get_proof_if_available(js);
                if (original_expr == nullptr) continue;

                bool has_precondition = false;
                expr *replacement = original_expr;
                for (auto *sub: info.relevant_expr) {
                    expr *precondition = find_precondition(sub, info.call, info.call.out_args[info.arg_idx]);
                    if (precondition == nullptr) continue;
                    has_precondition = true;
                    replacement = replace_expr(replacement, sub, precondition);
                }
                if (!has_precondition) continue;

                m.inc_ref(replacement);
//                ctx.internalize(replacement, is_quantifier(replacement));

                proof *replacement_proof = reinterpret_cast<proof *>(replacement);
                auto *js_casted = dynamic_cast<justification_proof_wrapper *>(js);
                js_casted->m_proof = replacement_proof;
            }
        }

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

//                for (unsigned l_idx = 0; l_idx < ctx.m_watches.size(); l_idx++) {
//                    literal l = to_literal(l_idx);
//                    auto &wl = const_cast<watch_list &>(ctx.m_watches[l.index()]);
//                    watch_list::clause_iterator it = wl.begin_clause();
//                    watch_list::clause_iterator end = wl.end_clause();
//                    for (; it != end; ++it) {
//                        clause *cls = *it;
//                        if (cls == nullptr) continue;
//                        if (visited_cls.find(cls) == visited_cls.end()) {
//                            visited_cls.emplace(cls);
//                            literal *lits = cls->begin();
//                            for (unsigned li = 0; li < cls->get_num_literals(); li++) {
//                                literal lit = lits[li];
//                                if (visited_expr.find(lit.var()) != visited_expr.end()) continue;
//                                visited_expr.emplace(lit.var());
//                                expr_ref e = ctx.literal2expr(lit);
//                                to_analyze.push_back(e.get());
//                            }
//                        }
//                        justification *js = cls->get_justification();
//                        if (visited_js.find(js) == visited_js.end()) {
//                            visited_js.emplace(js);
//                            proof *js_proof = get_proof_if_available(js);
//                            if (js_proof == nullptr) continue;
//                            to_analyze.push_back(js_proof);
//                        }
//                    }
//                }

                for (expr *e: to_analyze) {
                    ptr_vector<expr> sub = least_logical_subexpr_containing_expr(e, call.out_args[ai]);
                    axiom_expr_info info{sub, ai, call};
                    relevant_exprs.push_back(info);

//                    ptr_vector<expr> negated_sub = least_logical_negated_subexpr_containing_expr(e, call.out_args[ai]);
//                    expr_info info2{negated_sub, ai, call};
//                    relevant_exprs.push_back(info2);
                }
            }
        }

        for (auto &&info : relevant_exprs) {
            for (auto &&e: info.exprs) {
                expr *precondition = find_precondition(e, info.call, info.call.out_args[info.arg_idx]);
                if (precondition == nullptr) continue;

//                std::cout << "Find precondition: \n " << mk_pp(e, m) << "\n" << mk_pp(precondition, m) << "\n" << std::endl;

                ctx.internalize(precondition, is_quantifier(precondition));
                ctx.mark_as_relevant(precondition);
                m.inc_ref(precondition);


//                expr *lit_expr = m.mk_implies_simplified(precondition, e);
//                literal lit = mk_literal(lit_expr);
//                mk_th_axiom(lit);
//                visited_expr.emplace(lit.var());


                literal lit = mk_eq(e, precondition, is_quantifier(precondition));
                mk_th_axiom(lit);

//                literal lhs = mk_literal(precondition);
//                lhs.neg();
//                literal rhs = mk_literal(e);
//
//                visited_expr.emplace(lhs.var());
//                visited_expr.emplace(rhs.var());
//
//                ctx.mk_th_axiom(get_id(), lhs, rhs);
//
//                lhs.neg();
//                rhs.neg();
//                ctx.mk_th_axiom(get_id(), lhs, rhs);


//                literal lits[2] = { lhs, rhs};
//                justification* js = ctx.mk_justification(theory_axiom_justification(get_id(), ctx.get_region(), 2, lits));

//                proof* prf = m.mk_def_axiom(m.mk_implies(precondition, e));
//                m.inc_ref(prf);
//                justification* js = ctx.mk_justification(justification_proof_wrapper(ctx, prf));
////
//                bool_var_data& rhs_b_data = ctx.get_bdata(rhs.var());
//                ctx.set_justification(rhs.var(), rhs_b_data, b_justification(js));
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

    ptr_vector<expr> theory_function_call::least_logical_negated_subexpr_containing_expr(expr *e, expr *target) {
        ptr_vector<expr> result;
        llsce(m, e, target, nullptr, result, true);
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


    expr *
    theory_function_call::find_precondition(expr *expression, theory_function_call::call_info &call, expr *argument) {
        if (m.is_not(expression)) {
            expr *negated_expr = to_app(expression)->get_arg(0);
            expr *negated_precondition = find_precondition(negated_expr, call, argument);
            if (negated_precondition == nullptr) return nullptr;
            return m.mk_not(negated_precondition);
        }

        std::stringstream expr_str_builder;
        expr_str_builder << mk_pp(expression, m);
        auto &&expr_str = expr_str_builder.str();

        arith_util _arith(m);
        expr *x = _arith.mk_add(call.in_args[0], _arith.mk_int(5));

        if (expr_str == "(<= query!0_2_n 17)") {
            return _arith.mk_le(x, _arith.mk_int(17));
        } if (expr_str == "(<= query!0_2_n 99)") {
            return _arith.mk_le(x, _arith.mk_int(99));
        } else if (expr_str == "(>= (+ query!0_2_n (* (- 1) recursion_0_1)) (- 1))") {
            expr *recursion_var = to_app(to_app(to_app(expression)->get_arg(0))->get_arg(1))->get_arg(1);
            return _arith.mk_ge(_arith.mk_add(x, _arith.mk_mul(_arith.mk_int(-1), recursion_var)), _arith.mk_int(-1));
        } else if (expr_str == "(<= (+ query!0_2_n (* (- 1) recursion_0_1)) (- 1))") {
            expr *recursion_var = to_app(to_app(to_app(expression)->get_arg(0))->get_arg(1))->get_arg(1);
            return _arith.mk_le(_arith.mk_add(x, _arith.mk_mul(_arith.mk_int(-1), recursion_var)), _arith.mk_int(-1));
        } else if (expr_str == "(= (+ query!0_2_n (* (- 1) recursion_0_1)) (- 1))") {
            expr *recursion_var = to_app(to_app(to_app(expression)->get_arg(0))->get_arg(1))->get_arg(1);
            return _arith.mk_eq(_arith.mk_add(x, _arith.mk_mul(_arith.mk_int(-1), recursion_var)), _arith.mk_int(-1));
        } else if (expr_str == "(function_call query!0_3_n query!0_0_n query!0_2_n)") {
            return m.mk_true();
        } else if (expr_str == "(= recursion_0_1 (+ 1 query!0_2_n))") {
            expr *recursion_var = to_app(expression)->get_arg(0);
            return m.mk_eq(recursion_var, _arith.mk_add(_arith.mk_int(1), x));
        } else if (expr_str == "(<= (+ recursion_0_1 (* (- 1) (+ 1 query!0_2_n))) 0)") {
            expr *recursion_var = to_app(to_app(expression)->get_arg(0))->get_arg(0);
            expr *some = _arith.mk_mul(_arith.mk_int(-1), _arith.mk_add(_arith.mk_int(1), x));
            return _arith.mk_le(_arith.mk_add(recursion_var, some), _arith.mk_int(0));
        } else if (expr_str == "(>= (+ recursion_0_1 (* (- 1) (+ 1 query!0_2_n))) 0)") {
            expr *recursion_var = to_app(to_app(expression)->get_arg(0))->get_arg(0);
            expr *some = _arith.mk_mul(_arith.mk_int(-1), _arith.mk_add(_arith.mk_int(1), x));
            return _arith.mk_ge(_arith.mk_add(recursion_var, some), _arith.mk_int(0));
        } else if (expr_str == "(<= aux!3_n 17)") {
            return _arith.mk_le(x, _arith.mk_int(17));
        } else if (expr_str == "(function_call aux!2_n recursion_0_n aux!3_n)") {
            return m.mk_true();
        } else if (expr_str == "(= (+ recursion_0_0 (* (- 1) aux!3_n)) 1)") {
            expr *recursion_var = to_app(to_app(expression)->get_arg(0))->get_arg(0);
            return m.mk_eq(_arith.mk_add(recursion_var, _arith.mk_mul(_arith.mk_int(-1), x)), _arith.mk_int(1));
        } else if (expr_str == "(= recursion_0_0 (+ 1 aux!3_n))") {
            expr *recursion_var = to_app(expression)->get_arg(0);
            return m.mk_eq(recursion_var, _arith.mk_add(_arith.mk_int(1), x));
        } else if (expr_str == "(<= (+ recursion_0_0 (* (- 1) aux!3_n)) 1)") {
            expr *recursion_var = to_app(to_app(expression)->get_arg(0))->get_arg(0);
            return _arith.mk_le(_arith.mk_add(recursion_var, _arith.mk_mul(_arith.mk_int(-1), x)), _arith.mk_int(1));
        } else if (expr_str == "(>= (+ recursion_0_0 (* (- 1) aux!3_n)) 1)") {
            expr *recursion_var = to_app(to_app(expression)->get_arg(0))->get_arg(0);
            return _arith.mk_ge(_arith.mk_add(recursion_var, _arith.mk_mul(_arith.mk_int(-1), x)), _arith.mk_int(1));
        } else if (expr_str == "(<= (+ recursion_0_0 (* (- 1) (+ 1 aux!3_n))) 0)") {
            expr *recursion_var = to_app(to_app(expression)->get_arg(0))->get_arg(0);
            expr *some = _arith.mk_mul(_arith.mk_int(-1), _arith.mk_add(_arith.mk_int(1), x));
            return _arith.mk_le(_arith.mk_add(recursion_var, some), _arith.mk_int(0));
        } else if (expr_str == "(>= (+ recursion_0_0 (* (- 1) (+ 1 aux!3_n))) 0)") {
            expr *recursion_var = to_app(to_app(expression)->get_arg(0))->get_arg(0);
            expr *some = _arith.mk_mul(_arith.mk_int(-1), _arith.mk_add(_arith.mk_int(1), x));
            return _arith.mk_ge(_arith.mk_add(recursion_var, some), _arith.mk_int(0));
        } else {
            std::cout << "Unexpected lemma: " << expr_str << " Call: " << mk_pp(call.call, m) << std::endl;
            return nullptr;
        }
    }

    void theory_function_call::replace_literal(literal &lit, theory_function_call::call_info &call, expr *argument) {
        expr *original_expr = ctx.bool_var2expr(lit.var());
        expr *new_expr = find_precondition(original_expr, call, argument);
        if (new_expr == nullptr) return;

        ctx.internalize(new_expr, true);
        ctx.mark_as_relevant(new_expr);
        m.inc_ref(new_expr);
        ctx.m_bool_var2expr[lit.var()] = new_expr;

        ctx.display_literal_smt2(std::cout, lit) << std::endl;
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

    expr *theory_function_call::find_precondition_for_expr(expr *e) {
        expr *result = e;
        for (auto &&call : registered_calls) {
            for (unsigned ai = 0; ai < call.out_args.size(); ai++) {
                ptr_vector<expr> sub = least_logical_subexpr_containing_expr(result, call.out_args[ai]);
                for (auto &&sub_e: sub) {
                    expr *precondition = find_precondition(sub_e, call, call.out_args[ai]);
                    if (precondition == nullptr) continue;
                    result = replace_expr(result, sub_e, precondition);
                }
            }
        }
        return result;
    }

}
