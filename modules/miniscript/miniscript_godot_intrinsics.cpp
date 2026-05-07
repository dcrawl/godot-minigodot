#include "miniscript_godot_intrinsics.h"
#include "miniscript_instance.h"
#include "miniscript_script.h"
#include "miniscript_value_bridge.h"

#include "core/string/string_name.h"
#include "core/object/object.h"
#include "scene/main/node.h"

// Include full MiniScript headers AFTER Godot headers.
// Note: 'using namespace MiniScript' is NOT used here to avoid ambiguity
// between Godot::String and MiniScript::String.
#include "MiniscriptInterpreter.h"
#include "MiniscriptIntrinsics.h"
#include "MiniscriptTAC.h"

// _godot_emit(signal_name, a0, a1, a2, a3, a4, a5, a6, a7)
// Emits a signal on the current instance owner.
// Uses interpreter->hostData (set to MiniScriptInstance* before each call).
static MiniScript::IntrinsicResult intrinsic_godot_emit(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) {
        return MiniScript::IntrinsicResult::Null;
    }

    MiniScriptInstance *inst = static_cast<MiniScriptInstance *>(context->vm->interpreter->hostData);
    if (!inst) {
        return MiniScript::IntrinsicResult::Null;
    }

    MiniScript::Value signalNameVal = context->GetVar("signal_name");
    if (signalNameVal.IsNull()) {
        return MiniScript::IntrinsicResult::Null;
    }
    MiniScript::String ms_sig = signalNameVal.ToString();
    ::String signal_name = ::String::utf8(ms_sig.c_str());

    const char *arg_names[] = { "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7" };
    Vector<Variant> emit_args;
    for (int i = 0; i < 8; i++) {
        MiniScript::Value v = context->GetVar(arg_names[i]);
        if (v.IsNull()) {
            break;
        }
        emit_args.push_back(MiniScriptBridge::to_variant(v, context->vm));
    }

    // Validate signal argument count against the declared signal.
    Ref<MiniScriptScript> ms_script = inst->get_script();
    if (ms_script.is_valid()) {
        int expected = ms_script->get_signal_argument_count(StringName(signal_name));
        if (expected >= 0) {
            int got = emit_args.size();
            if (got != expected) {
                MiniScript::RuntimeException(MiniScript::String(
                    vformat("signal '%s' expected %d argument(s), got %d", signal_name, expected, got).utf8().get_data()
                )).raise();
            }
        }
    }

    Object *owner = inst->get_mini_owner();
    if (owner) {
        Vector<const Variant *> arg_ptrs;
        arg_ptrs.resize(emit_args.size());
        for (int i = 0; i < emit_args.size(); i++) {
            arg_ptrs.write[i] = &emit_args[i];
        }
        owner->emit_signalp(StringName(signal_name), arg_ptrs.ptrw(), emit_args.size());
    }

    return MiniScript::IntrinsicResult::Null;
}

// godot_call(method, a0..a7) — call a method on the owner node and return the result.
static MiniScript::IntrinsicResult intrinsic_godot_call(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) return MiniScript::IntrinsicResult::Null;
    MiniScriptInstance *inst = static_cast<MiniScriptInstance *>(context->vm->interpreter->hostData);
    if (!inst) return MiniScript::IntrinsicResult::Null;
    Object *owner = inst->get_mini_owner();
    if (!owner) return MiniScript::IntrinsicResult::Null;

    MiniScript::Value method_val = context->GetVar("method");
    if (method_val.IsNull()) return MiniScript::IntrinsicResult::Null;
    MiniScript::String ms_method = method_val.ToString();
    StringName method_name(::String::utf8(ms_method.c_str()));

    const char *arg_names[] = { "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7" };
    Vector<Variant> call_args;
    for (int i = 0; i < 8; i++) {
        MiniScript::Value v = context->GetVar(arg_names[i]);
        if (v.IsNull()) break;
        call_args.push_back(MiniScriptBridge::to_variant(v, context->vm));
    }
    Vector<const Variant *> call_arg_ptrs;
    call_arg_ptrs.resize(call_args.size());
    for (int i = 0; i < call_args.size(); i++) call_arg_ptrs.write[i] = &call_args[i];

    Callable::CallError cerr;
    Variant result = owner->callp(method_name, call_arg_ptrs.ptrw(), call_args.size(), cerr);
    if (cerr.error != Callable::CallError::CALL_OK) return MiniScript::IntrinsicResult::Null;
    return MiniScript::IntrinsicResult(MiniScriptBridge::to_ms(result));
}

// godot_get(property) — get a property from the owner node.
static MiniScript::IntrinsicResult intrinsic_godot_get(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) return MiniScript::IntrinsicResult::Null;
    MiniScriptInstance *inst = static_cast<MiniScriptInstance *>(context->vm->interpreter->hostData);
    if (!inst) return MiniScript::IntrinsicResult::Null;
    Object *owner = inst->get_mini_owner();
    if (!owner) return MiniScript::IntrinsicResult::Null;

    MiniScript::Value prop_val = context->GetVar("property");
    if (prop_val.IsNull()) return MiniScript::IntrinsicResult::Null;
    MiniScript::String ms_prop = prop_val.ToString();
    StringName prop_name(::String::utf8(ms_prop.c_str()));

    bool valid = false;
    Variant result = owner->get(prop_name, &valid);
    if (!valid) return MiniScript::IntrinsicResult::Null;
    return MiniScript::IntrinsicResult(MiniScriptBridge::to_ms(result));
}

// godot_set(property, value) — set a property on the owner node.
static MiniScript::IntrinsicResult intrinsic_godot_set(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) return MiniScript::IntrinsicResult::Null;
    MiniScriptInstance *inst = static_cast<MiniScriptInstance *>(context->vm->interpreter->hostData);
    if (!inst) return MiniScript::IntrinsicResult::Null;
    Object *owner = inst->get_mini_owner();
    if (!owner) return MiniScript::IntrinsicResult::Null;

    MiniScript::Value prop_val = context->GetVar("property");
    MiniScript::Value new_val = context->GetVar("value");
    if (prop_val.IsNull()) return MiniScript::IntrinsicResult::Null;
    MiniScript::String ms_prop = prop_val.ToString();
    StringName prop_name(::String::utf8(ms_prop.c_str()));

    owner->set(prop_name, MiniScriptBridge::to_variant(new_val, context->vm));
    return MiniScript::IntrinsicResult::Null;
}

// Helper: resolve a MiniScript number to a live Godot Object via ObjectDB.
static Object *get_object_by_ms_id(MiniScript::Value id_val) {
    if (id_val.IsNull() || id_val.type != MiniScript::ValueType::Number) return nullptr;
    uint64_t oid = (uint64_t)(int64_t)id_val.DoubleValue();
    if (oid == 0) return nullptr;
    return ObjectDB::get_instance(ObjectID(oid));
}

// godot_get_node(path) — get a child node of the owner and return its ObjectID as a number.
static MiniScript::IntrinsicResult intrinsic_godot_get_node(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) return MiniScript::IntrinsicResult::Null;
    MiniScriptInstance *inst = static_cast<MiniScriptInstance *>(context->vm->interpreter->hostData);
    if (!inst) return MiniScript::IntrinsicResult::Null;
    Node *owner_node = Object::cast_to<Node>(inst->get_mini_owner());
    if (!owner_node) return MiniScript::IntrinsicResult::Null;

    MiniScript::Value path_val = context->GetVar("path");
    if (path_val.IsNull()) return MiniScript::IntrinsicResult::Null;
    MiniScript::String ms_path = path_val.ToString();
    NodePath node_path(::String::utf8(ms_path.c_str()));

    Node *target = owner_node->get_node_or_null(node_path);
    if (!target) return MiniScript::IntrinsicResult::Null;

    uint64_t oid = (uint64_t)target->get_instance_id();
    return MiniScript::IntrinsicResult(MiniScript::Value((double)(int64_t)oid));
}

// godot_call_on(id, method, a0..a7) — call a method on an object by its ObjectID.
static MiniScript::IntrinsicResult intrinsic_godot_call_on(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) return MiniScript::IntrinsicResult::Null;

    Object *target = get_object_by_ms_id(context->GetVar("id"));
    if (!target) return MiniScript::IntrinsicResult::Null;

    MiniScript::Value method_val = context->GetVar("method");
    if (method_val.IsNull()) return MiniScript::IntrinsicResult::Null;
    StringName method_name(::String::utf8(method_val.ToString().c_str()));

    const char *arg_names[] = { "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7" };
    Vector<Variant> call_args;
    for (int i = 0; i < 8; i++) {
        MiniScript::Value v = context->GetVar(arg_names[i]);
        if (v.IsNull()) break;
        call_args.push_back(MiniScriptBridge::to_variant(v, context->vm));
    }
    Vector<const Variant *> call_arg_ptrs;
    call_arg_ptrs.resize(call_args.size());
    for (int i = 0; i < call_args.size(); i++) call_arg_ptrs.write[i] = &call_args[i];

    Callable::CallError cerr;
    Variant result = target->callp(method_name, call_arg_ptrs.ptrw(), call_args.size(), cerr);
    if (cerr.error != Callable::CallError::CALL_OK) return MiniScript::IntrinsicResult::Null;
    return MiniScript::IntrinsicResult(MiniScriptBridge::to_ms(result));
}

// godot_get_prop(id, property) — get a property from an object by its ObjectID.
static MiniScript::IntrinsicResult intrinsic_godot_get_prop(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) return MiniScript::IntrinsicResult::Null;

    Object *target = get_object_by_ms_id(context->GetVar("id"));
    if (!target) return MiniScript::IntrinsicResult::Null;

    MiniScript::Value prop_val = context->GetVar("property");
    if (prop_val.IsNull()) return MiniScript::IntrinsicResult::Null;
    StringName prop_name(::String::utf8(prop_val.ToString().c_str()));

    bool valid = false;
    Variant result = target->get(prop_name, &valid);
    if (!valid) return MiniScript::IntrinsicResult::Null;
    return MiniScript::IntrinsicResult(MiniScriptBridge::to_ms(result));
}

// godot_set_prop(id, property, value) — set a property on an object by its ObjectID.
static MiniScript::IntrinsicResult intrinsic_godot_set_prop(MiniScript::Context *context, MiniScript::IntrinsicResult partialResult) {
    if (!context || !context->vm || !context->vm->interpreter) return MiniScript::IntrinsicResult::Null;

    Object *target = get_object_by_ms_id(context->GetVar("id"));
    if (!target) return MiniScript::IntrinsicResult::Null;

    MiniScript::Value prop_val = context->GetVar("property");
    MiniScript::Value new_val = context->GetVar("value");
    if (prop_val.IsNull()) return MiniScript::IntrinsicResult::Null;
    StringName prop_name(::String::utf8(prop_val.ToString().c_str()));

    target->set(prop_name, MiniScriptBridge::to_variant(new_val, context->vm));
    return MiniScript::IntrinsicResult::Null;
}

void MiniScriptGodotIntrinsics::init_godot_intrinsics() {
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("_godot_emit");
        f->AddParam("signal_name");
        f->AddParam("a0", MiniScript::Value::null);
        f->AddParam("a1", MiniScript::Value::null);
        f->AddParam("a2", MiniScript::Value::null);
        f->AddParam("a3", MiniScript::Value::null);
        f->AddParam("a4", MiniScript::Value::null);
        f->AddParam("a5", MiniScript::Value::null);
        f->AddParam("a6", MiniScript::Value::null);
        f->AddParam("a7", MiniScript::Value::null);
        f->code = &intrinsic_godot_emit;
    }
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("godot_call");
        f->AddParam("method");
        f->AddParam("a0", MiniScript::Value::null);
        f->AddParam("a1", MiniScript::Value::null);
        f->AddParam("a2", MiniScript::Value::null);
        f->AddParam("a3", MiniScript::Value::null);
        f->AddParam("a4", MiniScript::Value::null);
        f->AddParam("a5", MiniScript::Value::null);
        f->AddParam("a6", MiniScript::Value::null);
        f->AddParam("a7", MiniScript::Value::null);
        f->code = &intrinsic_godot_call;
    }
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("godot_get");
        f->AddParam("property");
        f->code = &intrinsic_godot_get;
    }
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("godot_set");
        f->AddParam("property");
        f->AddParam("value", MiniScript::Value::null);
        f->code = &intrinsic_godot_set;
    }
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("godot_get_node");
        f->AddParam("path");
        f->code = &intrinsic_godot_get_node;
    }
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("godot_call_on");
        f->AddParam("id");
        f->AddParam("method");
        f->AddParam("a0", MiniScript::Value::null);
        f->AddParam("a1", MiniScript::Value::null);
        f->AddParam("a2", MiniScript::Value::null);
        f->AddParam("a3", MiniScript::Value::null);
        f->AddParam("a4", MiniScript::Value::null);
        f->AddParam("a5", MiniScript::Value::null);
        f->AddParam("a6", MiniScript::Value::null);
        f->AddParam("a7", MiniScript::Value::null);
        f->code = &intrinsic_godot_call_on;
    }
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("godot_get_prop");
        f->AddParam("id");
        f->AddParam("property");
        f->code = &intrinsic_godot_get_prop;
    }
    {
        MiniScript::Intrinsic *f = MiniScript::Intrinsic::Create("godot_set_prop");
        f->AddParam("id");
        f->AddParam("property");
        f->AddParam("value", MiniScript::Value::null);
        f->code = &intrinsic_godot_set_prop;
    }
}
