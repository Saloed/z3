//
// Created by napst on 04.02.2021.
//
#pragma once

#include "util/hashtable.h"
#include "util/mutex.h"
#include "util/event_handler.h"
#include "ast/ast.h"
#include "ast/arith_decl_plugin.h"
#include "ast/bv_decl_plugin.h"
#include "ast/seq_decl_plugin.h"
#include "ast/datatype_decl_plugin.h"
#include "ast/dl_decl_plugin.h"
#include "ast/fpa_decl_plugin.h"
#include "ast/recfun_decl_plugin.h"
#include "ast/special_relations_decl_plugin.h"
#include "ast/rewriter/seq_rewriter.h"
#include "smt/params/smt_params.h"
#include "smt/smt_kernel.h"
#include "smt/smt_solver.h"
#include "cmd_context/tactic_manager.h"
#include "params/context_params.h"
#include "cmd_context/cmd_context.h"
#include "solver/solver.h"
#include "api/z3.h"
#include "api/api_util.h"
#include "api/api_polynomial.h"

namespace api {
    class function_call_analyzer {
    public:
        explicit function_call_analyzer(api::context *context, function_call_analyzer_backend *analyzer);

        expr *
        find_precondition(expr *expression, unsigned function_id, expr **in_args, unsigned num_in_args, expr **out_args,
                          unsigned num_out_args);

        virtual ~function_call_analyzer();

        function_call_analyzer_backend *m_analyzer;
        api::context *m_api_context;
    };
}