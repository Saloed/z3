#pragma once

#include "z3.h"

class function_call_analyzer_backend {
public:
    virtual Z3_ast analyze(Z3_context ctx, Z3_ast e, unsigned function_id, Z3_ast const *in_args, unsigned num_in_args,
                           Z3_ast const *out_args, unsigned num_out_args) = 0;

    virtual ~function_call_analyzer_backend() {};
};


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/**
\brief Register function call analyzer

def_API('Z3_register_function_call_analyzer', BOOL, (_in(CONTEXT), _in(VOID_PTR)), has_special_native_impl=True)
*/
bool Z3_API Z3_register_function_call_analyzer(Z3_context c, void * analyzer_backend);

/**
\brief Provide function call info

def_API('Z3_provide_function_call_info', VOID, (_in(CONTEXT), _in(UINT), _in_array(1, UINT), _in_array(1, UINT), _in_array(1, UINT)))
*/
void Z3_API Z3_provide_function_call_info(Z3_context c, unsigned num_functions, unsigned function_ids[], unsigned function_number_in_args[], unsigned function_number_out_args[]);


#ifdef __cplusplus
}
#endif // __cplusplus
