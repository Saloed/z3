//
// Created by napst on 05.11.2020.
//
#pragma once

#include "ast/ast.h"

enum function_call_op_kind {
    OP_FUNCTION_CALL
};

class function_call_decl_plugin : public decl_plugin {
    symbol m_call_sym;

public:
    function_call_decl_plugin();

    decl_plugin *mk_fresh() override {
        return alloc(function_call_decl_plugin);
    }

    sort *mk_sort(decl_kind k, unsigned num_parameters, parameter const *parameters) override;

    func_decl *mk_func_decl(decl_kind k, unsigned num_parameters, parameter const *parameters,
                            unsigned arity, sort *const *domain, sort *range) override;

    void get_op_names(svector<builtin_name> &op_names, symbol const &logic) override;

};
