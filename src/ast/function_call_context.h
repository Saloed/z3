#pragma once

#include "ast.h"

namespace function_call{

    class call_info {
    public:
        call_info(expr_ref call_expr, expr_ref_vector in_args, expr_ref_vector out_args);

        expr_ref call_expr;
        expr_ref_vector in_args;
        expr_ref_vector out_args;
    };

    class function_call_precondition_miner {
    public:
        expr *find_precondition(expr *e, expr *call, ast_manager &m);

    private:
        expr *replace_expr(expr *original, expr *from, expr *to, ast_manager &m);
    };

    class function_call_context {
    public:
        expr_ref mk_call_axiom_for_expr(expr *e, call_info &call, ast_manager &m);

        expr_ref_vector generated_axioms(expr *call, ast_manager &m);

        void register_call(expr *call);

        call_info expand_call(expr* call, ast_manager& m);

    private:
        obj_map<expr, expr *> get_axioms(expr *call);

        obj_map<expr, obj_map<expr, expr *>> map;
        function_call_precondition_miner precondition_miner;
    };

    class function_call_context_provider {
    public:
        static function_call_context *get_context();

    private:
        static void initialize();

        static function_call_context *m_ctx;
    };
}


