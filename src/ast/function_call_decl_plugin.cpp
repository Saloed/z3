//
// Created by napst on 05.11.2020.
//

#include "function_call_decl_plugin.h"

function_call_decl_plugin::function_call_decl_plugin() :
        m_call_sym("function_call") {
    std::cout << "Create function call decl plugin" << std::endl;
}

sort *function_call_decl_plugin::mk_sort(decl_kind k, unsigned num_parameters, parameter const *parameters) {
    return nullptr; // No specific sort
}


func_decl *function_call_decl_plugin::mk_func_decl(decl_kind k, unsigned num_parameters, parameter const *parameters,
                                                   unsigned arity, sort *const *domain, sort *range) {
    SASSERT(k == OP_FUNCTION_CALL);
    if (arity < 1) {
        m_manager->raise_exception("call takes at least single argument");
        return nullptr;
    }
    return m_manager->mk_func_decl(m_call_sym, arity, domain, m_manager->mk_bool_sort(),
                                   func_decl_info(m_family_id, OP_FUNCTION_CALL));
}


void function_call_decl_plugin::get_op_names(svector<builtin_name> &op_names, symbol const &logic) {
    op_names.push_back(builtin_name("function_call", OP_FUNCTION_CALL));
}
