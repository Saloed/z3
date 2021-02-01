#pragma once

#include <api/api_context.h>
#include "ast.h"
#include "expr_map.h"
#include "util/vector.h"
#include "util/map.h"

namespace function_call {

    class call_axiom_storage {
    public:
        explicit call_axiom_storage(ast_manager &m);

        expr_map *get_call_axioms(expr *call);

        virtual ~call_axiom_storage();

    private:
        u_map<unsigned> calls;
        vector<expr_map *> axioms;
        ast_manager &m;
    };

    class expanded_call {
    public:
        expanded_call(unsigned id, expr_ref call_expr, expr_ref_vector in_args, expr_ref_vector out_args);

        unsigned id;
        expr_ref call_expr;
        expr_ref_vector in_args;
        expr_ref_vector out_args;
    };

    struct call_info {
        unsigned id;
        unsigned num_in_args;
        unsigned num_out_args;
    };

    class function_call_context {
    public:
        explicit function_call_context(ast_manager &m);

        expr_ref mk_call_axiom_for_expr(expr *e, expanded_call &call);

        expr_ref_vector generated_axioms(expr *call);

        void register_call(expr *call);

        expanded_call expand_call(expr *call);

        expr *find_function_call(expr *expression);

        void extend_forms_with_generated_axioms(expr_ref_vector &forms);

        unsigned get_function_id(expr *call);

        void update_function_call_analyzer(api::function_call_analyzer *analyzer);

        void update_call_info(unsigned int num_functions, const unsigned int function_ids[],
                              const unsigned int function_number_in_args[],
                              const unsigned int function_number_out_args[]);

    private:
        void extend_forms_with_generated_axioms(expr *form, expr_ref_vector &forms);

        expr *analyze_function_call(expanded_call &call, expr *expression);

        call_axiom_storage axiom_storage;
        api::function_call_analyzer *m_function_call_analyzer;
        u_map<function_call::call_info> call_info;

        ast_manager &m;
        family_id function_call_family_id;
    };
}


