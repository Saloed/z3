#pragma once

#include "ast.h"

class function_call_precondition_miner {
public:
    expr *find_precondition(expr *e, expr *call, ast_manager &m);

private:
    expr *replace_expr(expr *original, expr *from, expr *to, ast_manager& m);
};

class function_call_context {
public:
    expr_ref mk_call_axiom_for_expr(expr *e, expr *call, ast_manager &m) {
        auto &&call_axioms = get_axioms(call);
        expr *current_axiom = nullptr;
        if (call_axioms.find(e, current_axiom)) {
            return expr_ref(current_axiom, m);
        }
        current_axiom = precondition_miner.find_precondition(e, call, m);
        if (current_axiom == nullptr) {
            return expr_ref(m);
        }

        call_axioms.insert(e, current_axiom);
        map.insert(call, call_axioms);

        return expr_ref(current_axiom, m);
    }

    expr_ref_vector generated_axioms(expr *call, ast_manager &m) {
        auto &&current = get_axioms(call);
        expr_ref_vector result(m);
        for (auto &&axiom: current) {
            result.push_back(m.mk_eq(axiom.m_key, axiom.m_value));
        }
        return result;
    }

private:

    obj_map<expr, expr *> get_axioms(expr *call) {
        return map.contains(call) ? map[call] : obj_map<expr, expr *>();
    }

    obj_map<expr, obj_map<expr, expr *>> map;
    function_call_precondition_miner precondition_miner;
};


class function_call_context_provider {
public:
    static function_call_context *get_context() {
        if (!m_ctx) { initialize(); }
        return m_ctx;
    }

    static void initialize();

private:
    static function_call_context *m_ctx;
};