#pragma once

#include "muz/base/dl_context.h"
#include "muz/base/dl_rule_set.h"
#include "muz/base/dl_rule_transformer.h"
#include "tactic/equiv_proof_converter.h"

namespace datalog {

    class mk_function_call_normalize : public rule_transformer::plugin {
        context& m_ctx;
        ast_manager& m;
        rule_manager& rm;

    public:
        mk_function_call_normalize(context& ctx, unsigned priority);

        ~mk_function_call_normalize() override;

        rule_set* operator()(const rule_set& source) override;

    private:
        bool normalize(rule& r, rule_set& new_rules);

        template<typename VAR_GEN>
        expr* normalize_function_calls(expr* expression, VAR_GEN&& var_generator,
                                       function_call::function_call_context* const f_ctx);

        template<typename VAR_GEN>
        expr* normalize_function_call(app* function_call, VAR_GEN&& var_generator);
    };
}
