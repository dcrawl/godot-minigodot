#pragma once

// Bridge between Godot Variant and MiniScript::Value.
// Include this header ONLY in .cpp files that also include MiniScript headers.

#include "core/variant/variant.h"
#include "MiniscriptTypes.h"
#include "MiniscriptTAC.h"

namespace MiniScriptBridge {

// Convert a Godot Variant to a MiniScript::Value.
inline MiniScript::Value to_ms(const Variant &v) {
    switch (v.get_type()) {
        case Variant::NIL:
            return MiniScript::Value::null;
        case Variant::BOOL:
            return MiniScript::Value::Truth((bool)v);
        case Variant::INT:
            return MiniScript::Value((double)(int64_t)v);
        case Variant::FLOAT:
            return MiniScript::Value((double)(double)v);
        case Variant::STRING: {
            String gs = (String)v;
            CharString cs = gs.utf8();
            return MiniScript::Value(MiniScript::String(cs.get_data()));
        }
        default: {
            // For complex types, convert to string representation
            String gs = v.stringify();
            CharString cs = gs.utf8();
            return MiniScript::Value(MiniScript::String(cs.get_data()));
        }
    }
}

// Convert a MiniScript::Value to a Godot Variant.
// Takes non-const Value because MiniScript::Value::ToString is not const.
inline Variant to_variant(MiniScript::Value v, MiniScript::Machine *vm = nullptr) {
    switch (v.type) {
        case MiniScript::ValueType::Null:
            return Variant();
        case MiniScript::ValueType::Number: {
            double d = v.DoubleValue();
            int64_t i = (int64_t)d;
            if ((double)i == d) return Variant(i);
            return Variant(d);
        }
        default: {
            MiniScript::String s = v.ToString(vm);
            return Variant(String::utf8(s.c_str()));
        }
    }
}

} // namespace MiniScriptBridge
