//
// Created by napst on 05.11.2020.
//

#include "theory_function_call.h"
#include "smt/smt_context.h"

namespace smt {
    theory_function_call::theory_function_call(context &ctx) :
            theory(ctx, ctx.get_manager().mk_family_id("function_call")),
            m_find(*this),
            m_trail_stack(*this),
            m_final_check_idx(0) {
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
        SASSERT(r == static_cast<int>(m_var_data.size()));
        m_var_data.push_back(alloc(var_data));
        var_data *d = m_var_data[r];
        d->m_is_function = is_function_call_sort(n);
        d->m_is_function_call = is_function_call(n);
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
        add_parent_function_call(v_arg, ctx.get_enode(n));
        return true;
    }

    void theory_function_call::add_parent_function_call(theory_var v, enode *s) {
        SASSERT(is_function_call(s));
        v = find(v);
        var_data *d = m_var_data[v];
        d->m_parent_function_calls.push_back(s);
        m_trail_stack.push(push_back_trail<theory_function_call, enode *, false>(d->m_parent_function_calls));
    }


    void theory_function_call::new_eq_eh(theory_var v1, theory_var v2) {
        m_find.merge(v1, v2);
        enode *n1 = get_enode(v1), *n2 = get_enode(v2);
        assert_congruent(n1, n2);
    }

    void theory_function_call::assert_congruent(enode *a1, enode *a2) {
        SASSERT(is_function_call_sort(a1));
        SASSERT(is_function_call_sort(a2));
        if (a1->get_owner_id() > a2->get_owner_id())
            std::swap(a1, a2);
        enode *nodes[2] = {a1, a2};
        if (!ctx.add_fingerprint(this, 1, 2, nodes))
            return; // axiom was already instantiated
        m_equality_todo.push_back(std::make_pair(a1, a2));
    }


    void theory_function_call::new_diseq_eh(theory_var v1, theory_var v2) {
        v1 = find(v1);
        v2 = find(v2);
        var_data *d1 = m_var_data[v1];
        if (d1->m_is_function) {
            SASSERT(m_var_data[v2]->m_is_function);
            instantiate_extensionality(get_enode(v1), get_enode(v2));
        }
    }

    void theory_function_call::instantiate_extensionality(enode *a1, enode *a2) {
        SASSERT(is_function_call_sort(a1));
        SASSERT(is_function_call_sort(a2));
        assert_extensionality(a1, a2);
    }


    bool theory_function_call::assert_extensionality(enode *n1, enode *n2) {
        if (n1->get_owner_id() > n2->get_owner_id())
            std::swap(n1, n2);
        enode *nodes[2] = {n1, n2};
        if (!ctx.add_fingerprint(this, 0, 2, nodes))
            return false; // axiom was already instantiated
        m_extensionality_todo.push_back(std::make_pair(n1, n2));
        return true;
    }

    void theory_function_call::relevant_eh(app *n) {
        if (!ctx.e_internalized(n)) ctx.internalize(n, false);
        enode *arg = ctx.get_enode(n->get_arg(0));
        theory_var v_arg = arg->get_th_var(get_id());
        SASSERT(v_arg != null_theory_var);
        enode *e = ctx.get_enode(n);
        SASSERT(is_function_call(n));
        add_parent_function_call(v_arg, e);
    }

    void theory_function_call::push_scope_eh() {
        theory::push_scope_eh();
        m_trail_stack.push_scope();
    }

    void theory_function_call::reset_queues() {
        m_extensionality_todo.reset();
        m_equality_todo.reset();
    }

    void theory_function_call::pop_scope_eh(unsigned int num_scopes) {
        m_trail_stack.pop_scope(num_scopes);
        unsigned num_old_vars = get_old_num_vars(num_scopes);
        std::for_each(m_var_data.begin() + num_old_vars, m_var_data.end(), delete_proc<var_data>());
        m_var_data.shrink(num_old_vars);
        reset_queues();
        theory::pop_scope_eh(num_scopes);
        SASSERT(m_find.get_num_vars() == m_var_data.size());
        SASSERT(m_find.get_num_vars() == get_num_vars());
    }

    void theory_function_call::init_search_eh() {
        m_final_check_idx = 0;
    }

    void theory_function_call::restart_eh() {
        theory::restart_eh();
    }

    final_check_status theory_function_call::final_check_eh() {
        m_final_check_idx++;
        return FC_DONE; // todo
    }

    bool theory_function_call::is_shared(theory_var v) const {
        return false; // maybe todo
    }

    bool theory_function_call::can_propagate() {
        return !m_equality_todo.empty() || !m_extensionality_todo.empty();
    }

    void theory_function_call::propagate() {
        int a = 17;
        // todo: main logic
    }

    void theory_function_call::merge_eh(theory_var v1, theory_var v2, theory_var, theory_var) {
        // v1 is the new root
        SASSERT(v1 == find(v1));
        var_data *d2 = m_var_data[v2];
        for (unsigned i = 0; i < d2->m_parent_function_calls.size(); ++i)
            add_parent_function_call(v1, d2->m_parent_function_calls[i]);
    }

    void theory_function_call::after_merge_eh(theory_var r1, theory_var r2, theory_var v1, theory_var v2) {
        // do nothing
    }

    void theory_function_call::unmerge_eh(theory_var v1, theory_var v2) {
        // do nothing
    }

}
