#include <utility>
#include <vector>
#include <string>
#include "function_call_context.h"
#include "ast_pp.h"
#include "function_call_decl_plugin.h"
#include "api/api_context_function_call.h"


unsigned parse_id(expr *id_expr, ast_manager &m) {
    // fixme: rewrite this
    std::stringstream ss;
    ss << mk_pp(id_expr, m);
    std::string id_str = ss.str();
    return std::stoi(id_str);
}

unsigned function_call::function_call_context::get_function_id(expr *call) {
    unsigned call_id = call->get_id();
    if (f_call_to_f_id.contains(call_id)) {
        return f_call_to_f_id[call_id];
    }
    unsigned call_decl_id = to_app(call)->get_decl()->get_id();
    if (f_decl_to_f_id.contains(call_decl_id)) {
        unsigned f_id = f_decl_to_f_id[call_decl_id];
        f_call_to_f_id.insert(call_id, f_id);
        return f_id;
    }

    XXX("Unregistered call: " << call_id << "\n")
    expr *id_expr = to_app(call)->get_arg(0);
    unsigned f_id = parse_id(id_expr, m);
    f_call_to_f_id.insert(call_id, f_id);
    f_decl_to_f_id.insert(call_decl_id, f_id);
    return f_id;
}

void function_call::function_call_context::register_call(expr *call) {
    axiom_storage.get_call_axioms(call);
}

expr_ref_vector function_call::function_call_context::generated_axioms(expr *call) {
    auto &&current = axiom_storage.get_call_axioms(call);
    expr_ref_vector result(m);
    for (auto &&it = current->begin(); it != current->end(); ++it) {
        result.push_back(m.mk_eq(it->m_key, it->m_value));
    }
    return result;
}

expr_ref function_call::function_call_context::mk_call_axiom_for_expr(expr *e, expanded_call &call_info) {

    auto &&call_axioms = axiom_storage.get_call_axioms(call_info.call_expr.get());
    if (call_axioms->contains(e)) {
        expr *axiom;
        proof *p;
        call_axioms->get(e, axiom, p);
        return expr_ref(axiom, m);
    }

    expr *current_axiom = analyze_function_call(call_info, e);
    if (current_axiom == nullptr) {
        return expr_ref(m);
    }

    XXX("Generate axiom:\nSource: " << mk_pp(e, m) << "\nAxiom: " << mk_pp(current_axiom, m) << "\n")

    call_axioms->insert(e, current_axiom, nullptr);
    return expr_ref(current_axiom, m);
}

void expand_call_args(unsigned call_id, expr *call, expr_ref_vector &in_args, expr_ref_vector &out_args) {
    if (call_id == 777) {
        in_args.push_back(to_app(call)->get_arg(1));
        out_args.push_back(to_app(call)->get_arg(2));
    } else if (call_id == 123) {
        in_args.push_back(to_app(call)->get_arg(1));
        in_args.push_back(to_app(call)->get_arg(2));
        out_args.push_back(to_app(call)->get_arg(3));
    } else {
        std::cout << "Expand: unexpected call id " << call_id << std::endl;
    }
}

function_call::expanded_call function_call::function_call_context::expand_call(expr *call) {
    register_call(call);

    expr_ref call_expr_ref(call, m);
    expr_ref_vector in_args(m);
    expr_ref_vector out_args(m);
    unsigned id = get_function_id(call);

    expand_call_args(id, call, in_args, out_args);

    return expanded_call(id, call_expr_ref, in_args, out_args);
}

function_call::function_call_context::function_call_context(ast_manager &m)
        : m(m), m_function_call_analyzer(nullptr), axiom_storage(m) {
    function_call_family_id = m.mk_family_id("function_call");
    std::cout << "Create function call context" << std::endl;
}

expr *function_call::function_call_context::find_function_call(expr *root) {
    if (!is_app(root)) return nullptr;
    app *root_as_app = to_app(root);
    if (root_as_app->is_app_of(function_call_family_id, OP_FUNCTION_CALL)) {
        return root_as_app;
    }
    expr *result = nullptr;

    for (unsigned i = 0; i < root_as_app->get_num_args(); ++i) {
        expr *arg = root_as_app->get_arg(i);
        result = find_function_call(arg);
        if (result != nullptr) break;
    }
    return result;
}

void function_call::function_call_context::extend_forms_with_generated_axioms(expr_ref_vector &forms) {
    for (auto &&form: forms) {
        extend_forms_with_generated_axioms(form, forms);
    }
}

void function_call::function_call_context::extend_forms_with_generated_axioms(expr *form, expr_ref_vector &forms) {
    expr *function_call = find_function_call(form);
    if (function_call == nullptr) return;
    auto &&registered = generated_axioms(function_call);
    for (auto &&e: registered) {
        forms.push_back(e);
    }
}

void function_call::function_call_context::update_function_call_analyzer(api::function_call_analyzer *analyzer) {
    m_function_call_analyzer = analyzer;
    XXX("Successfully register function call analyzer: " << analyzer << "\n")
}

expr *function_call::function_call_context::analyze_function_call(expanded_call &call, expr *expression) {
    if (m_function_call_analyzer == nullptr) {
        UNREACHABLE();
        return nullptr;
    }
    expr *result = m_function_call_analyzer->find_precondition(
            expression, call.id,
            call.in_args.c_ptr(), call.in_args.size(),
            call.out_args.c_ptr(), call.out_args.size()
    );

    return result;
}

void
function_call::function_call_context::update_call_info(
        unsigned int num_functions, const unsigned int *function_ids,
        const unsigned int *function_number_in_args,
        const unsigned int *function_number_out_args
) {
    for (unsigned i = 0; i < num_functions; i++) {
        unsigned f_id = function_ids[i];
        unsigned f_in_args = function_number_in_args[i];
        unsigned f_out_args = function_number_out_args[i];
        function_call::call_info info{f_id, f_in_args, f_out_args};
        call_info.insert(f_id, info);
    }
}

app *
function_call::function_call_context::mk_function_call(unsigned int function_id, unsigned int num_args, expr **args) {
    decl_plugin *plugin = m.get_plugin(function_call_family_id);
    arith_util _arith(m);
    expr *function_id_expr = _arith.mk_int(function_id);

    ptr_vector<expr> function_call_args;
    function_call_args.push_back(function_id_expr);
    for (unsigned i = 0; i < num_args; i++) {
        function_call_args.push_back(args[i]);
    }
    ptr_vector<sort> function_call_args_sorts;
    vector<parameter> params;
    for (auto &&arg: function_call_args) {
        function_call_args_sorts.push_back(m.get_sort(arg));
        params.push_back(parameter(arg));
    }

    func_decl *call_decl = plugin->mk_func_decl(OP_FUNCTION_CALL, params.size(), params.c_ptr(),
                                                function_call_args_sorts.size(), function_call_args_sorts.c_ptr(),
                                                m.mk_bool_sort());
    app *function_call = m.mk_app(call_decl, function_call_args.size(), function_call_args.c_ptr());

    f_decl_to_f_id.insert(call_decl->get_id(), function_id);
    f_call_to_f_id.insert(function_call->get_id(), function_id);

    return function_call;
}

function_call::expanded_call::expanded_call(unsigned id, expr_ref call_expr, expr_ref_vector in_args,
                                            expr_ref_vector out_args)
        : call_expr(std::move(call_expr)), in_args(std::move(in_args)), out_args(std::move(out_args)), id(id) {}

function_call::call_axiom_storage::call_axiom_storage(ast_manager &m) : m(m) {
}

expr_map *function_call::call_axiom_storage::get_call_axioms(expr *call) {
    unsigned call_id = call->get_id();
    unsigned call_axiom_idx;
    if (calls.find(call_id, call_axiom_idx)) {
        return axioms[call_axiom_idx];
    }
    call_axiom_idx = axioms.size();
    expr_map *axiom_map = alloc(expr_map, m, false);
    axioms.push_back(axiom_map);
    calls.insert(call_id, call_axiom_idx);
    return axioms[call_axiom_idx];
}

function_call::call_axiom_storage::~call_axiom_storage() {
    for (unsigned i = 0; i < axioms.size(); ++i) {
        expr_map *axiom_map = axioms[i];
        dealloc(axiom_map);
    }
}
