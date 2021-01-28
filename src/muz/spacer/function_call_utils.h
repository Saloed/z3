#pragma once

#include <ast/function_call_decl_plugin.h>
#include "spacer_context.h"
#include "spacer_util.h"

#include "ast/function_call_context.h"

namespace spacer {

    expr *find_function_call(expr *root, family_id fid) {
        if (!is_app(root)) return nullptr;
        app *root_as_app = to_app(root);
        if (root_as_app->is_app_of(fid, OP_FUNCTION_CALL)) {
            return root_as_app;
        }
        expr *result = nullptr;

        for (unsigned i = 0; i < root_as_app->get_num_args(); ++i) {
            expr *arg = root_as_app->get_arg(i);
            result = find_function_call(arg, fid);
            if (result != nullptr) break;
        }
        return result;
    }

    expr *find_function_call(expr *root, ast_manager &m) {
        family_id fc_id = m.mk_family_id("function_call");
        return find_function_call(root, fc_id);
    }

    void add_implicant_forms_if_needed(expr_ref_vector &forms) {
        expr *function_call = nullptr;
        for (auto &&form: forms) {
            if (function_call != nullptr) break;
            function_call = find_function_call(form, forms.m());
        }
        if (function_call == nullptr) return;

        auto &&registered = function_call::function_call_context_provider::get_context()->generated_axioms(function_call, forms.m());
        for (auto &&e:registered) {
            forms.push_back(e);
        }
    }
}
