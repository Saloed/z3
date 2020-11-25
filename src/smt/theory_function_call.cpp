//
// Created by napst on 05.11.2020.
//

#include <ast/expr_functors.h>
#include <ast/rewriter/expr_replacer.h>
#include <unordered_map>
#include "theory_function_call.h"
#include "smt/smt_context.h"
#include "smt_model_generator.h"

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

    void print_bv2expr(ptr_vector<expr> &bv2e, ast_manager &manager) {
        for (unsigned i = 0; i < bv2e.size(); ++i) {
            expr *e = bv2e[i];
            std::cout << i << ": " << mk_pp(e, manager) << "\n";
        }
        std::cout << std::flush;
    }

    void theory_function_call::propagate() {
        ptr_vector<clause> lemmas = ctx.get_lemmas();
        print_bv2expr(ctx.m_bool_var2expr, m);

//        log_clauses(ctx, ctx.m_aux_clauses, aux_history, m_propagate_idx, ".z3-aux_trace");
//        log_clauses(ctx, lemmas, lemma_history, m_propagate_idx, ".z3-lemma_trace");

        std::cout << "Propagate " << m_propagate_idx << std::endl;
        TRACE("xxx", tout << m_propagate_idx;);
        if (m_propagate_idx == 5) {
//            std::cout << "bv2e:\n";
//            ctx.display_bool_var_defs(std::cout);
//            std::cout << "\nEnodes:\n";
//            ctx.display_enode_defs(std::cout);
//            ctx.display(std::cout);
            std::cout << std::flush;
        }
        m_propagate_idx++;

//        std::ofstream ctx_trace("z3_ctx_trace.txt", std::ios_base::out | std::ios_base::app);
//        ctx.display(ctx_trace);
//        ctx_trace.close();



//        ptr_vector<clause> lemmas = ctx.m_aux_clauses;

        vector<expr_ref> lemmas_to_analyze;
        vector<call_info> calls_to_analyze;
        ptr_vector<expr> arg_to_analyze;
        vector<literal> relevant_literals;

//        std::cout << "Current lemmas:" << std::endl;
//        for (auto &&lemma: lemmas) {
//            ctx.display_clause_smt2(std::cout, *lemma) << std::endl;
//        }
//

        analyze_all_exprs();

//        for (auto &&call : registered_calls) {
//            for (expr *out_arg : call.out_args) {
//                contains_expr arg_matcher(m, out_arg);
//                for (auto &&lemma : lemmas) {
//                    for (unsigned i = 0; i < lemma->get_num_literals(); i++) {
//                        literal lemma_literal = lemma->get_literal(i);
//                        expr_ref literal_expr = ctx.literal2expr(lemma_literal);
//                        if (arg_matcher(literal_expr)) {
//                            relevant_literals.push_back(lemma_literal);
//                            lemmas_to_analyze.push_back(literal_expr);
//                            calls_to_analyze.push_back(call);
//                            arg_to_analyze.push_back(out_arg);
//                        }
//                    }
//                }
//            }
//            if (!call.is_asserted) {
//                literal call_literal = mk_literal(call.call);
//                mk_th_axiom(call_literal);
//                call.is_asserted = true;
//            }
//        }

//        for (unsigned i = 0; i < lemmas_to_analyze.size(); ++i) {
//            analyze_lemma(lemmas_to_analyze[i], calls_to_analyze[i], arg_to_analyze[i]);
//        }

        for (unsigned i = 0; i < relevant_literals.size(); ++i) {
//            replace_literal(relevant_literals[i], calls_to_analyze[i], arg_to_analyze[i]);
        }

        m_num_pending_queries = 0;
    }

    struct expr_info {
        unsigned bv;
        unsigned arg_idx;
        expr *relevant_expr;
        theory_function_call::call_info call;
    };

    void theory_function_call::analyze_all_exprs() {
        vector<expr_info> relevant_exprs;
        for (auto &&call : registered_calls) {
            for (unsigned ai = 0; ai < call.out_args.size(); ai++) {
                for (unsigned bv = 0; bv < ctx.m_bool_var2expr.size(); ++bv) {
                    expr *e = ctx.m_bool_var2expr[bv];
                    if (e == nullptr) continue;
                    ptr_vector<expr> sub = least_logical_subexpr_containing_expr(e, call.out_args[ai]);
                    for (expr *sub_expr: sub) {
                        expr_info info{bv, ai, sub_expr, call};
                        relevant_exprs.push_back(info);
                    }
                }
            }
        }

        for (unsigned i = 0; i < relevant_exprs.size(); i++) {
            expr_info info = relevant_exprs[i];
            std::cout << i
                      << " { " << mk_pp(ctx.bool_var2expr(info.bv), m) << " } "
                      << mk_pp(info.relevant_expr, m)
                      << std::endl;
            expr *precondition = find_precondition(info.relevant_expr, info.call, info.call.out_args[info.arg_idx]);
            if (precondition == nullptr) continue;
            expr *original_expr = ctx.m_bool_var2expr[info.bv];
            expr *replacement = replace_expr(original_expr, info.relevant_expr, precondition);
            ctx.internalize(replacement, true);
            ctx.mark_as_relevant(replacement);
            m.inc_ref(replacement);
            *original_expr = *replacement;
            ctx.m_bool_var2expr[info.bv] = replacement;
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

    void llsce(ast_manager &m, expr *e, expr *target, expr *nearest_logical_parent, ptr_vector<expr> &result) {
        bool is_logic = m.is_bool(e);
        if (e == target) {
            if (is_logic) {
                result.push_back(e);
                return;
            }
            if (nearest_logical_parent != nullptr) {
                result.push_back(nearest_logical_parent);
                return;
            }
            UNREACHABLE();
        }
        expr *new_parent = is_logic ? e : nearest_logical_parent;
        switch (e->get_kind()) {
            case AST_VAR:
                break;
            case AST_APP: {
                app *a = to_app(e);
                for (unsigned i = 0; i < a->get_num_args(); ++i) {
                    expr *arg = a->get_arg(i);
                    llsce(m, arg, target, new_parent, result);
                }
                break;
            }
            case AST_QUANTIFIER: {
                quantifier *q = to_quantifier(e);
                expr *arg = q->get_expr();
                llsce(m, arg, target, new_parent, result);
                break;
            }
            default: UNREACHABLE();
                break;
        }
    }

    ptr_vector<expr> theory_function_call::least_logical_subexpr_containing_expr(expr *e, expr *target) {
        ptr_vector<expr> result;
        llsce(m, e, target, nullptr, result);
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
        std::stringstream expr_str_builder;
        expr_str_builder << mk_pp(expression, m);
        auto &&expr_str = expr_str_builder.str();

        if (expr_str == "(<= query!0_2_n 17)") {
            std::cout << "Match expr 0" << std::endl;
            arith_util _arith(m);
            return _arith.mk_ge(call.in_args[0], _arith.mk_int(12));
        } else if (expr_str == "(>= (+ query!0_2_n (* (- 1) recursion_0_1)) (- 1))") {
            std::cout << "Match expr 1" << std::endl;
            expr *recursion_var = to_app(to_app(to_app(expression)->get_arg(0))->get_arg(1))->get_arg(1);
            arith_util _arith(m);
            expr *x = _arith.mk_add(call.in_args[0], _arith.mk_int(5));
            return _arith.mk_ge(_arith.mk_add(x, _arith.mk_mul(_arith.mk_int(-1), recursion_var)), _arith.mk_int(-1));
        } else if (expr_str == "(<= (+ query!0_2_n (* (- 1) recursion_0_1)) (- 1))") {
            std::cout << "Match expr 1" << std::endl;
            expr *recursion_var = to_app(to_app(to_app(expression)->get_arg(0))->get_arg(1))->get_arg(1);
            arith_util _arith(m);
            expr *x = _arith.mk_add(call.in_args[0], _arith.mk_int(5));
            return _arith.mk_le(_arith.mk_add(x, _arith.mk_mul(_arith.mk_int(-1), recursion_var)), _arith.mk_int(-1));
        } else if (expr_str == "(= (+ query!0_2_n (* (- 1) recursion_0_1)) (- 1))") {
            std::cout << "Match expr 1" << std::endl;
            expr *recursion_var = to_app(to_app(to_app(expression)->get_arg(0))->get_arg(1))->get_arg(1);
            arith_util _arith(m);
            expr *x = _arith.mk_add(call.in_args[0], _arith.mk_int(5));
            return _arith.mk_eq(_arith.mk_add(x, _arith.mk_mul(_arith.mk_int(-1), recursion_var)), _arith.mk_int(-1));
        } else if (expr_str == "(function_call query!0_3_n query!0_0_n query!0_2_n)") {
            return m.mk_true();
        } else {
            std::cout << "Unexpected lemma: " << expr_str << std::endl;
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


    void theory_function_call::analyze_lemma(expr_ref &lemma, theory_function_call::call_info &call, expr *argument) {
//        std::cout << "Analyze lemma literal: " << mk_pp(lemma, m) << std::endl;
        std::stringstream lemma_str_builder;
        lemma_str_builder << mk_pp(lemma, m);
        auto &&lemma_str = lemma_str_builder.str();
        expr *precondition = nullptr;
        // res = arg + 5
        if (lemma_str == "(not (<= query!0_2_n 17))") {
            if (call.checked_lemmas[0]) return;
//            std::cout << "Found lemma 0" << std::endl;
            arith_util _arith(m);
            precondition = _arith.mk_ge(call.in_args[0], _arith.mk_int(12));
            call.checked_lemmas[0] = true;
        } else if (lemma_str == "(not (>= (+ query!0_2_n (* (- 1) recursion_0_1)) (- 1)))") {
            if (call.checked_lemmas[1]) return;
//            std::cout << "Found lemma 1" << std::endl;
            expr *recursion_var = to_app(
                    to_app(
                            to_app(
                                    to_app(lemma.get())->get_arg(0)
                            )->get_arg(0)
                    )->get_arg(1)
            )->get_arg(1);

            arith_util _arith(m);
            expr *x = _arith.mk_add(call.in_args[0], _arith.mk_int(5));
            precondition = m.mk_not(
                    _arith.mk_ge(_arith.mk_add(x, _arith.mk_mul(_arith.mk_int(-1), recursion_var)), _arith.mk_int(-1))
            );
            call.checked_lemmas[1] = true;
        } else {
//            std::cout << "Unexpected lemma" << std::endl;
        }
        if (precondition == nullptr) return;
//        std::cout << "Precondition: " << mk_pp(precondition, m) << std::endl;

        expr_ref precondition_expr(precondition, get_manager());
        ctx.internalize(precondition_expr, false);

//        expr_ref pre_impl_lemma = mk_implies_simplified(precondition_expr, lemma);
//        literal x = mk_literal(pre_impl_lemma);
//
//        ctx.display_literal_smt2(std::cout, pre_impl_lemma_lit) << std::endl;
//
//        ctx.mark_as_relevant(precondition);
//        literal lits[1] = {pre_impl_lemma_lit};
//
//        ctx.mk_th_axiom(get_id(), 1, lits);
//        ctx.mk_th_lemma(get_id(), 1, lits);

//        ctx.internalize(precondition_expr, true);
//
//        literal precondition_literal = ctx.get_literal(precondition_expr);
//        literal postcondition_literal = ctx.get_literal(lemma);
//        literal call_literal = ctx.get_literal(call.call);
//        ctx.mark_as_relevant(precondition_literal);
//
//        ctx.mk_th_axiom(get_id(), precondition_literal, postcondition_literal);

        literal x = mk_eq(precondition_expr, lemma, false);
        mk_th_axiom(x);
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
