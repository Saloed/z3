//
// Created by napst on 04.02.2021.
//

#include "ast/function_call_context.h"
#include "api/api_log_macros.h"
#include "api/api_context_function_call.h"

namespace api {
    function_call_analyzer::function_call_analyzer(api::context *context, function_call_analyzer_backend *analyzer)
            : m_analyzer(analyzer), m_api_context(context) {
    }

    function_call_analyzer::~function_call_analyzer() {
        delete m_analyzer;  // allocated via new operator in api
        m_analyzer = nullptr;
    }

    bool check_function_call_enabled(api::context *ctx) {
        if (!ctx->function_calls_enabled()) {
            ctx->set_error_code(Z3_EXCEPTION, "Function calls support disabled");
            return false;
        }
        return true;
    }

    expr *function_call_analyzer::find_precondition(expr *expression, unsigned int function_id, expr **in_args,
                                                    unsigned int num_in_args, expr **out_args,
                                                    unsigned int num_out_args) const {
        if (!check_function_call_enabled(m_api_context)) return nullptr;
        XXX("Run analyzer for function " << function_id << "\n")
        struct _Z3_ast *result = m_analyzer->analyze(reinterpret_cast<struct _Z3_context *>(m_api_context),
                                                     of_expr(expression), function_id,
                                                     of_exprs(in_args), num_in_args,
                                                     of_exprs(out_args), num_out_args
        );
        return to_expr(result);
    }

    bool context::update_call_analyzer(function_call_analyzer_backend *analyzer) {
        if (!check_function_call_enabled(this)) return false;
        XXX("update analyzer with: " << analyzer << "\n")
        if (m_function_call_analyzer != nullptr) {
            XXX("Function call analyzer is not nullptr\n")
            return false;
        }
        m_function_call_analyzer = alloc(function_call_analyzer, this, analyzer);
        m_manager->get_function_call_context()->update_function_call_analyzer(m_function_call_analyzer);
        return true;
    }

    void context::update_function_call_info(unsigned int num_functions, const unsigned int function_ids[],
                                            const unsigned int function_number_in_args[],
                                            const unsigned int function_number_out_args[]) {
        if (!check_function_call_enabled(this)) return;
        XXX("update function call info" << "\n")
        m_manager->get_function_call_context()->update_call_info(
                num_functions, function_ids, function_number_in_args, function_number_out_args
        );
    }

    app *context::mk_function_call(unsigned int function_id, unsigned int num_args, expr **args) {
        if (!check_function_call_enabled(this)) return nullptr;
        return m().get_function_call_context()->mk_function_call(function_id, num_args, args);
    }

}