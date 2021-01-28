#include "util/max_cliques.h"
#include "util/stopwatch.h"
#include "ast/ast_util.h"
#include "ast/ast_pp.h"
#include "model/model_pp.h"
#include "smt/smt_context.h"
#include "theory_function_call.h"

namespace smt {

    std::ostream &display_theory(std::ostream &out, const context *ctx, theory_id th_id) {
        theory *th = ctx->get_theory(th_id);
        if (th != nullptr) {
            out << th->get_name() << "\n";
        } else {
            out << "Null theory: " << th_id << "\n";
        }
        return out;
    }

    std::ostream &context::display_justification(std::ostream &out, justification *j) {
//        out << j << " ";
        out << j->get_name() << "\n";
        if (strcmp(j->get_name(), "proof-wrapper") == 0) {
            auto *pw = dynamic_cast<smt::justification_proof_wrapper *>(j);
            out << mk_pp(pw->m_proof, get_manager()) << "\n";
        }
        if (strcmp(j->get_name(), "theory-axiom") == 0) {
            auto *axiom = dynamic_cast<smt::theory_axiom_justification *>(j);
            display_theory(out, this, axiom->get_from_theory());
            for (unsigned i = 0; i < axiom->m_num_literals; i++) {
                display_literal_smt2(out, axiom->m_literals[i]) << "\n";
            }
        }
        if (strcmp(j->get_name(), "theory-lemma") == 0) {
            auto *lemma = dynamic_cast<smt::theory_lemma_justification *>(j);
            display_theory(out, this, lemma->get_from_theory());
            for (unsigned i = 0; i < lemma->m_num_literals; i++) {
                bool sign = GET_TAG(lemma->m_literals[i]) != 0;
                expr *v = UNTAG(expr*, lemma->m_literals[i]);
                expr *res = sign ? m.mk_not(v) : v;
                out << mk_pp(res, m) << "\n";
            }
        }
        if (strcmp(j->get_name(), "unit-resolution") == 0) {
            literal_vector literals;
            get_cr().justification2literals(j, literals);
            display_literals_smt2(out, literals);
        }
        return out;
    }

    std::ostream &context::display_justification(std::ostream &out, const b_justification &js) {
        out << js.get_kind() << " | ";
        switch (js.get_kind()) {
            case b_justification::CLAUSE: {
                clause *cls = js.get_clause();
                if (cls != nullptr) {
                    out << "\n";
                    display_clause_smt2(out, *cls) << "\nJustification is cls:\n";
                    display_justification(out, cls->get_justification()) << std::endl;
                } else {
                    out << "null" << std::endl;
                }
                break;
            }
            case b_justification::BIN_CLAUSE: {
                display_literal_smt2(out, js.get_literal()) << std::endl;
                break;
            }
            case b_justification::AXIOM: {
                break;
            }
            case b_justification::JUSTIFICATION: {
                justification *j = js.get_justification();
                display_justification(out, j) << std::endl;
                break;
            }
        }
        return out;
    }

    void context::display_watches_smt2(std::ostream &out) {
        unsigned s = m_watches.size();
        for (unsigned l_idx = 0; l_idx < s; l_idx++) {
            literal l = to_literal(l_idx);
            display_watch_list_smt2(out, l);
            out << "\n";
        }
        out << std::endl;
    }

    void context::display_watch_list_smt2(std::ostream &out, literal &l) {
        out << "Var: " << l.var() << " watch_list:\n";
        auto &wl = const_cast<watch_list &>(m_watches[l.index()]);
        watch_list::clause_iterator it = wl.begin_clause();
        watch_list::clause_iterator end = wl.end_clause();
        for (; it != end; ++it) {
            clause *cls = *it;
            display_clause_smt2(out, *cls) << "\nJustification in cls:\n";
            justification *js = cls->get_justification();
            display_justification(out, js) << "\n";
        }
    }

}