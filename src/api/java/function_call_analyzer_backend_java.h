//
// Created by napst on 01.02.2021.
//

#pragma once

#include <jni.h>
#include "z3.h"

class function_call_analyzer_backend_java : public function_call_analyzer_backend {
public:
    function_call_analyzer_backend_java(JNIEnv * jenv, jclass cls, jint analyzer_instance);

    Z3_ast
    analyze(Z3_context ctx, Z3_ast e, unsigned function_id, Z3_ast const *in_args, unsigned num_in_args,
            Z3_ast const *out_args, unsigned num_out_args) override;

    ~function_call_analyzer_backend_java() override { };

private:
    jmethodID get_analyzer_method();


    JNIEnv* env;
    jclass  m_native_cls;
    jint analyzer_id;
    jmethodID m_analyzer_method;
};

