//
// Created by napst on 05.11.2020.
//

#include <sstream>
#include "function_call_decl_plugin.h"

#include "ast/ast_pp.h"

#define FUNCTION_CALL_SORT_STR "FunctionCall"

function_call_decl_plugin::function_call_decl_plugin() :
        m_call_sym("function_call") {
}


sort *function_call_decl_plugin::mk_sort(decl_kind k, unsigned num_parameters, parameter const *parameters) {
    SASSERT(k == FUNCTION_CALL_SORT);

    for (unsigned i = 0; i < num_parameters; i++) {
        if (!parameters[i].is_ast() || !is_sort(parameters[i].get_ast())) {
            m_manager->raise_exception("invalid function call sort definition, parameter is not a sort");
            return nullptr;
        }
    }
    return m_manager->mk_sort(symbol(FUNCTION_CALL_SORT_STR),
                              sort_info(m_family_id, FUNCTION_CALL_SORT, 1, num_parameters, parameters));
}

bool function_call_decl_plugin::is_function_call_sort(sort *s) const {
    return m_family_id == s->get_family_id() && s->get_decl_kind() == FUNCTION_CALL_SORT;
}


func_decl *function_call_decl_plugin::mk_func_decl(decl_kind k, unsigned num_parameters, parameter const *parameters,
                                                   unsigned arity, sort *const *domain, sort *range) {
    SASSERT(k == OP_FUNCTION_CALL);
    return mk_function_call(arity, domain);
}


void function_call_decl_plugin::get_sort_names(svector<builtin_name> &sort_names, symbol const &logic) {
    sort_names.push_back(builtin_name(FUNCTION_CALL_SORT_STR, FUNCTION_CALL_SORT));
}

void function_call_decl_plugin::get_op_names(svector<builtin_name> &op_names, symbol const &logic) {
    op_names.push_back(builtin_name("function_call", OP_FUNCTION_CALL));
}


expr *function_call_decl_plugin::get_some_value(sort *s) {
    SASSERT(s->is_sort_of(m_family_id, FUNCTION_CALL_SORT));
    return m_manager->mk_true();
}

bool function_call_decl_plugin::is_fully_interp(sort *s) const {
    SASSERT(s->is_sort_of(m_family_id, FUNCTION_CALL_SORT));
    unsigned sz = s->get_num_parameters();
    for (unsigned i = 0; i < sz; i++) {
        if (!m_manager->is_fully_interp(to_sort(s->get_parameter(i).get_ast())))
            return false;
    }
    return true;
}

func_decl *function_call_decl_plugin::mk_function_call(unsigned int arity, sort *const *args) {
    if (arity < 1) {
        m_manager->raise_exception("call takes at least single argument");
        return nullptr;
    }
//    sort *s = args[0];
//    unsigned num_parameters = s->get_num_parameters();
//    parameter const *parameters = s->get_parameters();
//
//    if (num_parameters + 1 != arity) {
//        std::stringstream strm;
//        strm << "function call requires " << num_parameters + 1 << " arguments, but was provided with " << arity
//             << " arguments";
//        m_manager->raise_exception(strm.str());
//        return nullptr;
//    }
//    ptr_buffer<sort> new_domain; // we need this because of coercions.
//    new_domain.push_back(s);
//    for (unsigned i = 0; i < num_parameters; ++i) {
//        if (!parameters[i].is_ast() ||
//            !is_sort(parameters[i].get_ast()) ||
//            !m_manager->compatible_sorts(args[i + 1], to_sort(parameters[i].get_ast()))) {
//            std::stringstream strm;
//            strm << "argument sort " << sort_ref(args[i + 1], *m_manager) << " and parameter ";
//            strm << parameter_pp(parameters[i], *m_manager) << " do not match";
//            m_manager->raise_exception(strm.str());
//            return nullptr;
//        }
//        new_domain.push_back(to_sort(parameters[i].get_ast()));
//    }
//    SASSERT(new_domain.size() == arity);
//    return m_manager->mk_func_decl(m_call_sym, arity, new_domain.c_ptr(), m_manager->mk_bool_sort(),
//                                   func_decl_info(m_family_id, OP_FUNCTION_CALL));
    return m_manager->mk_func_decl(m_call_sym, arity, args, m_manager->mk_bool_sort(),
                                   func_decl_info(m_family_id, OP_FUNCTION_CALL));
}