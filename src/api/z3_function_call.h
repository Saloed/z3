#pragma once

#include "z3.h"

class FunctionCallAnalyzer {
public:
    FunctionCallAnalyzer(void *jvmEnv, void *analyzer);

public:
    Z3_ast analyze(Z3_ast e, unsigned function_id, Z3_ast const * in_args, unsigned num_in_args, Z3_ast const* out_args, unsigned num_out_args);

    void *jvm_env;
    void *analyzer_obj;
};


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/**
\brief Register function call analyzer (JAVA ONLY)

def_java_only_API('Z3_register_function_call_analyzer', BOOL, (_in(CONTEXT), _in(JAVA_ENV), _in(JAVA_OBJECT)))
*/
bool Z3_API Z3_register_function_call_analyzer(Z3_context c, void *jvm_env, void *analyzer);

/**
\brief Provide function call info

def_API('Z3_provide_function_call_info', VOID, (_in(CONTEXT), _in(UINT), _in_array(1, UINT), _in_array(1, UINT), _in_array(1, UINT)))
*/
void Z3_API Z3_provide_function_call_info(Z3_context c, unsigned num_functions, unsigned function_ids[], unsigned function_number_in_args[], unsigned function_number_out_args[]);


#ifdef __cplusplus
}
#endif // __cplusplus
