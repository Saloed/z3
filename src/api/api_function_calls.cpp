//
// Created by napst on 29.01.2021.
//

#include<jni.h>

#include "api/z3.h"
#include "api/api_context.h"
#include "z3_function_call.h"


extern "C" {
bool Z3_API Z3_register_function_call_analyzer(Z3_context c, void *jvm_env, void *analyzer) {
    FunctionCallAnalyzer new_analyzer(jvm_env, analyzer);
    auto &&ctx = mk_c(c);
    ctx->reset_error_code();
    return ctx->update_call_analyzer(new_analyzer);
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

FunctionCallAnalyzer::FunctionCallAnalyzer(void *jvmEnv, void *analyzer) : jvm_env(jvmEnv), analyzer_obj(analyzer) {
}

Z3_ast
FunctionCallAnalyzer::analyze(Z3_ast e, unsigned int function_id, Z3_ast const *in_args, unsigned int num_in_args,
                              Z3_ast const *out_args, unsigned int num_out_args) {
    JNIEnv *env = static_cast<JNIEnv *>(jvm_env);
    jobject obj = static_cast<jobject>(analyzer_obj);

    jclass analyzer_cls = env->GetObjectClass(obj);
    jmethodID analyze_method = env->GetMethodID(analyzer_cls, "analyze", "(Ljava/lang/Object)Ljava/lang/Object;");

    jobject result = (jobject) env->CallObjectMethod(obj, analyze_method);

    return nullptr;
}
