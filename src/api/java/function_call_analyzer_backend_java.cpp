//
// Created by napst on 01.02.2021.
//

#include "function_call_analyzer_backend_java.h"

bool has_pending_exception(JNIEnv *env) {
    if (!env->ExceptionCheck()) return false;
    env->ExceptionDescribe();
    env->ExceptionClear();
    fprintf(stderr, "Exception in function_call_analyzer_backend_java\n");
    return true;
}

Z3_ast
function_call_analyzer_backend_java::analyze(Z3_context ctx, Z3_ast e, unsigned function_id, Z3_ast const *in_args,
                                             unsigned num_in_args,
                                             Z3_ast const *out_args, unsigned num_out_args) {
    jmethodID analyzer_method = get_analyzer_method();
    if (analyzer_method == nullptr) return nullptr;

    jlongArray inArgsArray = env->NewLongArray(num_in_args);
    jlongArray outArgsArray = env->NewLongArray(num_out_args);

    env->SetLongArrayRegion(inArgsArray, 0, num_in_args, (jlong *) in_args);
    env->SetLongArrayRegion(outArgsArray, 0, num_out_args, (jlong *) out_args);

    if (has_pending_exception(env)) return nullptr;

    jlong result = env->CallStaticLongMethod(
            m_native_cls, analyzer_method,
            (jlong) ctx, analyzer_id, function_id,
            (jlong) e, inArgsArray, outArgsArray
    );

    if (has_pending_exception(env)) return nullptr;

    env->DeleteLocalRef(inArgsArray);
    env->DeleteLocalRef(outArgsArray);

    return (Z3_ast) result;
}

function_call_analyzer_backend_java::function_call_analyzer_backend_java(
        JNIEnv *jenv, jclass cls, jint analyzer_instance
) : env(jenv), analyzer_id(analyzer_instance), m_analyzer_method(nullptr), m_native_cls(nullptr) {
}

jmethodID function_call_analyzer_backend_java::get_analyzer_method() {
    if (m_analyzer_method == nullptr) {
        m_native_cls = env->FindClass("saloed/z3/Native");
        m_analyzer_method = env->GetStaticMethodID(
                m_native_cls,
                "INTERNALrunFunctionCallAnalyzer",
                "(JIIJ[J[J)J"
        );
    }
    return m_analyzer_method;
}
