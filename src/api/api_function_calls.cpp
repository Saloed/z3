//
// Created by napst on 29.01.2021.
//

#include "api/z3.h"
#include "api/api_context.h"
#include "z3_function_call.h"


extern "C" {
bool Z3_API Z3_register_function_call_analyzer(Z3_context c, void * raw_analyzer_backend){
    auto &&ctx = mk_c(c);
    ctx->reset_error_code();
    auto analyzer_backend = reinterpret_cast<function_call_analyzer_backend*>(raw_analyzer_backend);
    return ctx->update_call_analyzer(analyzer_backend);
}

void Z3_API Z3_provide_function_call_info(Z3_context c, unsigned num_functions, unsigned function_ids[],
                                          unsigned function_number_in_args[], unsigned function_number_out_args[]) {
    auto &&ctx = mk_c(c);
    ctx->reset_error_code();
    auto &&call_analyzer = ctx->get_call_analyzer();
    if (call_analyzer == nullptr) {
        ctx->set_error_code(Z3_EXCEPTION, "Function analyzer is not initialized");
        return;
    }
    call_analyzer->update_function_call_info(num_functions, function_ids, function_number_in_args,
                                             function_number_out_args);
}
}
