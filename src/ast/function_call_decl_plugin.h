//
// Created by napst on 05.11.2020.
//
#pragma once

#include "ast/ast.h"

enum function_call_sort_kind {
    FUNCTION_CALL_SORT
};

enum function_call_op_kind{
    OP_FUNCTION_CALL
};

class function_call_decl_plugin : public decl_plugin {
    symbol m_call_sym;

    func_decl *mk_function_call(unsigned arity, sort *const *args);

    bool is_function_call_sort(sort *s) const;

public:
    function_call_decl_plugin();

    ~function_call_decl_plugin() override {}

    decl_plugin *mk_fresh() override {
        return alloc(function_call_decl_plugin);
    }

    sort *mk_sort(decl_kind k, unsigned num_parameters, parameter const *parameters) override;

    func_decl *mk_func_decl(decl_kind k, unsigned num_parameters, parameter const *parameters,
                            unsigned arity, sort *const *domain, sort *range) override;


    void get_op_names(svector<builtin_name> &op_names, symbol const &logic) override;

    void get_sort_names(svector<builtin_name> &sort_names, symbol const &logic) override;

    expr *get_some_value(sort *s) override;

    bool is_fully_interp(sort *s) const override;
};