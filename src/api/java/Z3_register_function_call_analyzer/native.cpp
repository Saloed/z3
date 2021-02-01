DLL_VIS JNIEXPORT jboolean JNICALL Java_%s_Native_INTERNALregisterFunctionCallAnalyzer(JNIEnv * jenv, jclass cls, jlong a0, jobject a1) {
    function_call_analyzer_backend_java* backend = new function_call_analyzer_backend_java(jenv, cls, a1);
    bool result = Z3_register_function_call_analyzer((Z3_context)a0, backend);
    return (jboolean) result;
}
