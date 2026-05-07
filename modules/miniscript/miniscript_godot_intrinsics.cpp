#include "miniscript_godot_intrinsics.h"
#include "miniscript_instance.h"
#include "miniscript_script.h"
#include "miniscript_value_bridge.h"

#include "core/string/string_name.h"

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

void MiniScriptGodotIntrinsics::init_godot_intrinsics() {
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
