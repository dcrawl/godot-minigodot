#include "miniscript_instance.h"

#include "core/core_constants.h"
#include "core/debugger/engine_debugger.h"
#include "core/error/error_macros.h"
#include "core/math/math_defs.h"
#include "core/os/os.h"
#include "scene/main/node.h"
#include "miniscript_language.h"
#include "miniscript_script.h"
#include "miniscript_value_bridge.h"

#include "MiniscriptInterpreter.h"
#include "MiniscriptIntrinsics.h"
#include "MiniscriptTAC.h"

// Convenience helper: cast void* to MiniScript::Interpreter*.
static inline MiniScript::Interpreter *get_interp(void *ptr) {
    return static_cast<MiniScript::Interpreter *>(ptr);
}

// Named static callbacks matching TextOutputMethod signature: void(MiniScript::String, bool).
static void ms_std_output(MiniScript::String s, bool /*eol*/) {
    print_line(::String::utf8(s.c_str()));
}
static void ms_err_output(MiniScript::String s, bool /*eol*/) {
    ERR_PRINT("MiniScript runtime error: " + ::String::utf8(s.c_str()));
}

MiniScriptInstance::~MiniScriptInstance() {
    if (script.is_valid()) {
        script->_unregister_instance(this);
    }
    delete get_interp(ms_interp);
    ms_interp = nullptr;
}

void MiniScriptInstance::_reset_interpreter() {
    delete get_interp(ms_interp);
    ms_interp = nullptr;
    ms_interp_ready = false;
}

void MiniScriptInstance::snapshot_live_locals(Vector<String> &r_names, Vector<Variant> &r_values) const {
    MiniScript::Interpreter *interp = get_interp(ms_interp);
    if (!interp || !interp->vm) {
        return;
    }
    MiniScript::Context *ctx = interp->vm->GetTopContext();
    if (!ctx) {
        return;
    }
    MiniScript::ValueDict &vars = ctx->variables;
    if (vars.Count() == 0) {
        return;
    }
    for (auto it = vars.GetIterator(); !it.Done(); it.Next()) {
        MiniScript::Value key = it.Key();
        MiniScript::Value val = it.Value();
        MiniScript::String key_str = key.ToString(interp->vm);
        r_names.push_back(String::utf8(key_str.c_str()));
        r_values.push_back(MiniScriptBridge::to_variant(val, interp->vm));
    }
}

void MiniScriptInstance::_ensure_interpreter() {
    if (ms_interp_ready) {
        return;
    }

    if (!script.is_valid()) {
        return;
    }

    const String &src = script->get_preprocessed_source();
    if (src.is_empty()) {
        ms_interp_ready = true;
        return;
    }

    delete get_interp(ms_interp);
    MiniScript::Interpreter *interp = new MiniScript::Interpreter();
    ms_interp = interp;
    interp->hostData = this;

    interp->standardOutput = &ms_std_output;
    interp->errorOutput = &ms_err_output;

    CharString cs = src.utf8();
    interp->Reset(MiniScript::String(cs.get_data()));
    interp->Compile();

    // Run module-level code to define all functions in the global scope.
    interp->RunUntilDone(10.0);

    // Seed Godot global constants into the interpreter's global context.
    // Covers: built-in enums (OK, FAILED, ERR_*, KEY_*, etc.), math constants (PI, TAU, INF),
    // and autoload singletons registered via add_global_constant().
    // Seed Godot global constants into the interpreter's global context.
    // Covers: built-in enums (OK, FAILED, ERR_*, KEY_*, etc.), math constants (PI, TAU, INF),
    // and autoload singletons registered via add_global_constant().
    if (interp->vm) {
        // CoreConstants: integer enum values (OK=0, FAILED=1, ERR_*, etc.)
        const int core_count = CoreConstants::get_global_constant_count();
        for (int ci = 0; ci < core_count; ci++) {
            const char *name = CoreConstants::get_global_constant_name(ci);
            int64_t value = CoreConstants::get_global_constant_value(ci);
            interp->SetGlobalValue(MiniScript::String(name), MiniScript::Value((double)value));
        }

        // Math constants not in CoreConstants
        interp->SetGlobalValue(MiniScript::String("PI"),  MiniScript::Value(Math::PI));
        interp->SetGlobalValue(MiniScript::String("TAU"), MiniScript::Value(Math::TAU));
        interp->SetGlobalValue(MiniScript::String("INF"), MiniScript::Value((double)INFINITY));
        interp->SetGlobalValue(MiniScript::String("NAN"), MiniScript::Value((double)NAN));

        // Autoload singletons registered via add_global_constant()
        MiniScriptLanguage *lang = MiniScriptLanguage::get_singleton();
        if (lang) {
            for (const KeyValue<StringName, Variant> &kv : lang->get_global_constants()) {
                CharString key_cs = String(kv.key).utf8();
                interp->SetGlobalValue(MiniScript::String(key_cs.get_data()), MiniScriptBridge::to_ms(kv.value));
            }
        }
    }

    ms_interp_ready = true;
}

void MiniScriptInstance::_sync_props_to_interp() {
    MiniScript::Interpreter *interp = get_interp(ms_interp);
    if (!interp || !script.is_valid()) {
        return;
    }
    for (const KeyValue<StringName, Variant> &kv : property_values) {
        CharString cs = String(kv.key).utf8();
        MiniScript::String ms_name(cs.get_data());
        interp->SetGlobalValue(ms_name, MiniScriptBridge::to_ms(kv.value));
    }
}

void MiniScriptInstance::_sync_props_from_interp() {
    MiniScript::Interpreter *interp = get_interp(ms_interp);
    if (!interp || !script.is_valid()) {
        return;
    }
    for (KeyValue<StringName, Variant> &kv : property_values) {
        CharString cs = String(kv.key).utf8();
        MiniScript::String ms_name(cs.get_data());
        MiniScript::Value v = interp->GetGlobalValue(ms_name);
        if (!v.IsNull()) {
            kv.value = MiniScriptBridge::to_variant(v, interp->vm);
        }
    }
}

MiniScriptInstance::MiniScriptInstance(const Ref<MiniScriptScript> &p_script, Object *p_owner) {
    script = p_script;
    owner = p_owner;
    if (script.is_valid()) {
        script->_register_instance(this);
    }

    if (script.is_valid()) {
        List<PropertyInfo> properties;
        script->get_script_property_list(&properties);
        for (const PropertyInfo &property : properties) {
            Variant default_value;
            if (script->get_property_default_value(property.name, default_value)) {
                property_values.insert(property.name, default_value);
            } else {
                property_values.insert(property.name, Variant());
            }
        }
    }
}

Object *MiniScriptInstance::get_mini_owner() const {
    return owner;
}

bool MiniScriptInstance::get_mini_property_value(const StringName &p_name, Variant &r_value) const {
    if (const Variant *value = property_values.getptr(p_name)) {
        r_value = *value;
        return true;
    }
    return false;
}

bool MiniScriptInstance::set(const StringName &p_name, const Variant &p_value) {
    if (!script.is_valid() || !script->has_exported_property(p_name)) {
        return false;
    }

    property_values.insert(p_name, p_value);
    return true;
}

bool MiniScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
    if (const Variant *value = property_values.getptr(p_name)) {
        r_ret = *value;
        return true;
    }
    return false;
}

void MiniScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {
    if (script.is_valid()) {
        script->get_script_property_list(p_properties);
    }
}

Variant::Type MiniScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
    if (const Variant *value = property_values.getptr(p_name)) {
        if (r_is_valid != nullptr) {
            *r_is_valid = true;
        }
        return value->get_type();
    }

    if (r_is_valid != nullptr) {
        *r_is_valid = false;
    }

    return Variant::NIL;
}

void MiniScriptInstance::validate_property(PropertyInfo &p_property) const {
}

bool MiniScriptInstance::property_can_revert(const StringName &p_name) const {
    if (!script.is_valid()) {
        return false;
    }

    Variant default_value;
    return script->get_property_default_value(p_name, default_value);
}

bool MiniScriptInstance::property_get_revert(const StringName &p_name, Variant &r_ret) const {
    if (!script.is_valid()) {
        return false;
    }

    return script->get_property_default_value(p_name, r_ret);
}

void MiniScriptInstance::get_method_list(List<MethodInfo> *p_list) const {
    if (script.is_valid()) {
        script->get_script_method_list(p_list);
    }
}

bool MiniScriptInstance::has_method(const StringName &p_method) const {
    return script.is_valid() && script->has_method(p_method);
}

Variant MiniScriptInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
    if (!script.is_valid()) {
        r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
        return Variant();
    }

    // When a debugger is attached, use the custom executor (preserves all debug hooks).
    if (EngineDebugger::is_active()) {
        return script->call_script_method(p_method, p_args, p_argcount, r_error, this);
    }

    // Non-debug path: use the real MiniScript interpreter.
    if (!script->has_method(p_method)) {
        r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
        return Variant();
    }

    _ensure_interpreter();
    MiniScript::Interpreter *interp = get_interp(ms_interp);
    if (!interp) {
        // Interpreter unavailable (no source or compile error) — fall back to custom executor.
        return script->call_script_method(p_method, p_args, p_argcount, r_error, this);
    }

    // Sync instance properties to interpreter globals.
    _sync_props_to_interp();

    // Set hostData so _godot_emit intrinsic can access this instance.
    interp->hostData = this;

    // Build the REPL call expression: __ms_result__ = methodName(__ms_arg0__, ...)
    for (int i = 0; i < p_argcount; i++) {
        CharString arg_name_cs = vformat("__ms_arg%d__", i).utf8();
        MiniScript::String ms_arg_name(arg_name_cs.get_data());
        MiniScript::Value ms_arg = (p_args && p_args[i]) ? MiniScriptBridge::to_ms(*p_args[i]) : MiniScript::Value::null;
        interp->SetGlobalValue(ms_arg_name, ms_arg);
    }

    CharString method_cs = String(p_method).utf8();
    String call_expr = vformat("__ms_result__ = %s(", String::utf8(method_cs.get_data()));
    for (int i = 0; i < p_argcount; i++) {
        if (i > 0) {
            call_expr += ", ";
        }
        call_expr += vformat("__ms_arg%d__", i);
    }
    call_expr += ")";

    // Push a debug frame so debug_get_current_stack_info() returns live data
    // for any signal handlers that fire synchronously during REPL execution.
    const String script_path = script->get_path().is_empty() ? String("<memory>") : script->get_path();
    const int decl_line = script->get_method_declaration_line(p_method);
    Vector<String> arg_names;
    Vector<Variant> arg_values;
    arg_names.resize(p_argcount);
    arg_values.resize(p_argcount);
    for (int i = 0; i < p_argcount; i++) {
        arg_names.write[i] = vformat("arg%d", i);
        arg_values.write[i] = (p_args && p_args[i]) ? *p_args[i] : Variant();
    }
    MiniScriptLanguage::push_debug_frame(script_path, String(p_method), decl_line, arg_names, arg_values);

    // Snapshot exported properties into the debug frame so the debugger shows them.
    {
        Vector<String> prop_names;
        Vector<Variant> prop_vals;
        for (const KeyValue<StringName, Variant> &kv : property_values) {
            prop_names.push_back(String(kv.key));
            prop_vals.push_back(kv.value);
        }
        MiniScriptLanguage::update_debug_frame_members(prop_names, prop_vals);
    }

    CharString call_expr_cs = call_expr.utf8();
    interp->REPL(MiniScript::String(call_expr_cs.get_data()));

    MiniScriptLanguage::pop_debug_frame();

    // Sync properties back (method may have mutated them).
    _sync_props_from_interp();

    r_error.error = Callable::CallError::CALL_OK;

    // Get return value.
    MiniScript::Value result = interp->GetGlobalValue("__ms_result__");
    if (result.IsNull()) {
        return Variant();
    }
    return MiniScriptBridge::to_variant(result, interp->vm);
}

void MiniScriptInstance::notification(int p_notification, bool p_reversed) {
    if (!script.is_valid()) {
        return;
    }

    Callable::CallError call_error;
    auto report_call_error = [&call_error]() {
        if (call_error.error != Callable::CallError::CALL_OK) {
            String message = MiniScriptLanguage::get_singleton() ? MiniScriptLanguage::get_singleton()->debug_get_error() : String("MiniScript runtime error");
            ERR_PRINT(message);
        }
    };

    auto call_noarg_method = [&](const StringName &p_method) {
        if (!script->has_method(p_method)) {
            return;
        }
        callp(p_method, nullptr, 0, call_error);
        report_call_error();
    };

    auto call_delta_method = [&](const StringName &p_method, double p_delta) {
        if (!script->has_method(p_method)) {
            return;
        }

        Variant delta_arg = p_delta;
        const Variant *args[1] = { &delta_arg };
        callp(p_method, args, 1, call_error);
        report_call_error();
    };

    switch (p_notification) {
        case Node::NOTIFICATION_ENTER_TREE: {
            call_noarg_method(SNAME("_enter_tree"));
        } break;

        case Node::NOTIFICATION_READY: {
            call_noarg_method(SNAME("_ready"));
        } break;

        case Node::NOTIFICATION_PROCESS: {
            double delta = 0.0;
            if (Node *node = Object::cast_to<Node>(owner)) {
                delta = node->get_process_delta_time();
            }
            call_delta_method(SNAME("_process"), delta);
        } break;

        case Node::NOTIFICATION_PHYSICS_PROCESS: {
            double delta = 0.0;
            if (Node *node = Object::cast_to<Node>(owner)) {
                delta = node->get_physics_process_delta_time();
            }
            call_delta_method(SNAME("_physics_process"), delta);
        } break;

        case Node::NOTIFICATION_EXIT_TREE: {
            call_noarg_method(SNAME("_exit_tree"));
        } break;

        default: {
            // Generic _notification(what) handler for all other notifications.
            if (script->has_method(SNAME("_notification"))) {
                Variant what_arg = p_notification;
                const Variant *args[1] = { &what_arg };
                callp(SNAME("_notification"), args, 1, call_error);
                report_call_error();
            }
        } break;
    }
}

Object *MiniScriptInstance::get_owner() {
    return owner;
}

Ref<Script> MiniScriptInstance::get_script() const {
    return script;
}

ScriptLanguage *MiniScriptInstance::get_language() {
    if (script.is_valid()) {
        return script->get_language();
    }
    return MiniScriptLanguage::get_singleton();
}

const Variant MiniScriptInstance::get_rpc_config() const {
    if (script.is_valid()) {
        return script->get_rpc_config();
    }
    return Variant();
}
