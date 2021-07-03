#pragma once

#include "ast.h"
#include "expr_map.h"
#include "util/map.h"
#include "util/vector.h"
#include "api/api_context_function_call.h"
#include "ast/rewriter/th_rewriter.h"

namespace function_call {

    class call_axiom_storage {
    public:
        explicit call_axiom_storage(ast_manager& m);

        expr_map* get_call_axioms(expr* call);

        virtual ~call_axiom_storage();

        std::ostream& display(std::ostream& out);

    private:
        u_map<unsigned> calls;
        vector<expr_map*> axioms;
        ast_manager& m;
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
        explicit function_call_context(ast_manager& m);

        expr_ref mk_call_axiom_for_expr(expr* e, expanded_call& call);

        expr_ref_vector generated_axioms(expr* call);

        void register_call(expr* call);

        expanded_call expand_call(expr* call);

        expr* find_function_call(expr* expression);

        void extend_forms_with_generated_axioms(expr_ref_vector& forms);

        unsigned get_function_id(expr* call);

        void update_function_call_analyzer(api::function_call_analyzer* analyzer);

        void update_call_info(unsigned int num_functions, const unsigned int function_ids[],
                              const unsigned int function_number_in_args[],
                              const unsigned int function_number_out_args[]);

        app* mk_function_call(unsigned int function_id, unsigned int num_args, expr** args);

        bool is_function_call(expr* e) const;

    private:
        void extend_forms_with_generated_axioms(expr* form, expr_ref_vector& forms);

        expr* analyze_function_call(expanded_call& call, expr* expression);
        expr_ref prepare_generated_axiom(expr* e);

        void expand_call_args(unsigned int call_id, expr* call, expr_ref_vector& in_args, expr_ref_vector& out_args);
        
        call_axiom_storage axiom_storage;
        api::function_call_analyzer* m_function_call_analyzer;
        u_map<function_call::call_info> call_info;

        u_map<unsigned> f_decl_to_f_id;
        u_map<unsigned> f_call_to_f_id;
        u_map<bool> expressions_with_no_axiom;

        ast_manager& m;
        family_id function_call_family_id;

        unsigned analyzer_calls = 0;
    };
}
