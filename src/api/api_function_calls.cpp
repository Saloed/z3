//
// Created by napst on 29.01.2021.
//

#include "api/z3.h"
#include "api/api_context.h"
#include "z3_function_call.h"
#include "api/api_log_macros.h"


extern "C" {
bool Z3_API Z3_register_function_call_analyzer(Z3_context c, void *raw_analyzer_backend) {
    Z3_TRY ;
        LOG_Z3_register_function_call_analyzer(c, raw_analyzer_backend);
        RESET_ERROR_CODE();
        auto analyzer_backend = reinterpret_cast<function_call_analyzer_backend *>(raw_analyzer_backend);
        bool update_status = mk_c(c)->update_call_analyzer(analyzer_backend);
        return update_status;
    Z3_CATCH_RETURN(false);
}

void Z3_API Z3_provide_function_call_info(Z3_context c, unsigned num_functions, unsigned function_ids[],
                                          unsigned function_number_in_args[], unsigned function_number_out_args[]) {
    Z3_TRY ;
        LOG_Z3_provide_function_call_info(c, num_functions, function_ids, function_number_in_args,
                                          function_number_out_args);
        RESET_ERROR_CODE();
        mk_c(c)->update_function_call_info(num_functions, function_ids, function_number_in_args,
                                           function_number_out_args);
    Z3_CATCH;
}

Z3_ast Z3_API Z3_mk_function_call(Z3_context c, unsigned fid, unsigned num_args, Z3_ast const args[]) {
    Z3_TRY ;
        LOG_Z3_mk_function_call(c, fid, num_args, args);
        RESET_ERROR_CODE();
        ptr_buffer<expr> arg_list;
        for (unsigned i = 0; i < num_args; ++i) {
            arg_list.push_back(to_expr(args[i]));
        }
        app *a = mk_c(c)->mk_function_call(fid, arg_list.size(), arg_list.c_ptr());
        mk_c(c)->save_ast_trail(a);
        check_sorts(c, a);
        RETURN_Z3(of_ast(a));
    Z3_CATCH_RETURN(nullptr);
}

}
