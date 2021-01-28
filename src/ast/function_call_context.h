#pragma once

#include "ast.h"

namespace function_call {

    class call_info {
    public:
        call_info(expr_ref call_expr, expr_ref_vector in_args, expr_ref_vector out_args);

        expr_ref call_expr;
        expr_ref_vector in_args;
        expr_ref_vector out_args;
    };

    class function_call_precondition_miner {
    public:
        explicit function_call_precondition_miner(ast_manager &m);

        expr *find_precondition(expr *e, expr *call, unsigned call_id);

    private:
        expr *replace_expr(expr *original, expr *from, expr *to);

        ast_manager &m;
    };

    class function_call_context {
    public:
        explicit function_call_context(ast_manager &m);

        expr_ref mk_call_axiom_for_expr(expr *e, call_info &call);

        expr_ref_vector generated_axioms(expr *call);

        void register_call(expr *call);

        call_info expand_call(expr *call);

        expr *find_function_call(expr *expression);

        void extend_forms_with_generated_axioms(expr_ref_vector &forms);

        unsigned get_function_id(expr *call);

    private:
        void extend_forms_with_generated_axioms(expr *form, expr_ref_vector &forms);

        obj_map<expr, expr *> get_axioms(expr *call);

        obj_map<expr, obj_map<expr, expr *>> map;
        function_call_precondition_miner precondition_miner;
        ast_manager &m;

        family_id function_call_family_id;

    };
}


