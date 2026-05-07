#pragma once

// Bridge between Godot Variant and MiniScript::Value.
// Include this header ONLY in .cpp files that also include MiniScript headers.

#include "core/variant/variant.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/math/color.h"
#include "MiniscriptTypes.h"
#include "MiniscriptTAC.h"

namespace MiniScriptBridge {

// Build a MiniScript Map from key/value pairs (string key → number value).
inline MiniScript::Value make_vec_map(std::initializer_list<std::pair<const char*, double>> fields) {
    MiniScript::ValueDict d;
    for (auto &kv : fields) {
        d.SetValue(MiniScript::Value(MiniScript::String(kv.first)), MiniScript::Value(kv.second));
    }
    return MiniScript::Value(d);
}

// Try to read a numeric field from a MiniScript Map; returns 0 if missing.
inline double map_get_double(MiniScript::ValueDict &d, const char *key) {
    MiniScript::Value result;
    if (d.Get(MiniScript::Value(MiniScript::String(key)), &result)) {
        if (result.type == MiniScript::ValueType::Number) return result.DoubleValue();
    }
    return 0.0;
}

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
        case Variant::VECTOR2: {
            Vector2 vec = (Vector2)v;
            return make_vec_map({{"x", vec.x}, {"y", vec.y}});
        }
        case Variant::VECTOR2I: {
            Vector2i vec = (Vector2i)v;
            return make_vec_map({{"x", (double)vec.x}, {"y", (double)vec.y}});
        }
        case Variant::VECTOR3: {
            Vector3 vec = (Vector3)v;
            return make_vec_map({{"x", vec.x}, {"y", vec.y}, {"z", vec.z}});
        }
        case Variant::VECTOR3I: {
            Vector3i vec = (Vector3i)v;
            return make_vec_map({{"x", (double)vec.x}, {"y", (double)vec.y}, {"z", (double)vec.z}});
        }
        case Variant::COLOR: {
            Color c = (Color)v;
            return make_vec_map({{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}});
        }
        default: {
            // For complex types, convert to string representation
            String gs = v.stringify();
            CharString cs = gs.utf8();
            return MiniScript::Value(MiniScript::String(cs.get_data()));
        }
    }
}

// Detect if a MiniScript Map looks like a Vector2 (has x and y, no z/r/g/b).
inline bool is_vec2_map(MiniScript::ValueDict &d) {
    MiniScript::Value tmp;
    bool has_x = d.Get(MiniScript::Value(MiniScript::String("x")), &tmp);
    bool has_y = d.Get(MiniScript::Value(MiniScript::String("y")), &tmp);
    bool has_z = d.Get(MiniScript::Value(MiniScript::String("z")), &tmp);
    bool has_r = d.Get(MiniScript::Value(MiniScript::String("r")), &tmp);
    return has_x && has_y && !has_z && !has_r;
}

// Detect if a MiniScript Map looks like a Vector3 (has x, y, z).
inline bool is_vec3_map(MiniScript::ValueDict &d) {
    MiniScript::Value tmp;
    bool has_x = d.Get(MiniScript::Value(MiniScript::String("x")), &tmp);
    bool has_y = d.Get(MiniScript::Value(MiniScript::String("y")), &tmp);
    bool has_z = d.Get(MiniScript::Value(MiniScript::String("z")), &tmp);
    return has_x && has_y && has_z;
}

// Detect if a MiniScript Map looks like a Color (has r, g, b).
inline bool is_color_map(MiniScript::ValueDict &d) {
    MiniScript::Value tmp;
    bool has_r = d.Get(MiniScript::Value(MiniScript::String("r")), &tmp);
    bool has_g = d.Get(MiniScript::Value(MiniScript::String("g")), &tmp);
    bool has_b = d.Get(MiniScript::Value(MiniScript::String("b")), &tmp);
    return has_r && has_g && has_b;
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
        case MiniScript::ValueType::Map: {
            MiniScript::ValueDict d = v.GetDict();
            if (is_vec3_map(d)) {
                return Variant(Vector3(
                    (float)map_get_double(d, "x"),
                    (float)map_get_double(d, "y"),
                    (float)map_get_double(d, "z")));
            }
            if (is_color_map(d)) {
                MiniScript::Value tmp;
                double a = 1.0;
                if (d.Get(MiniScript::Value(MiniScript::String("a")), &tmp) && tmp.type == MiniScript::ValueType::Number)
                    a = tmp.DoubleValue();
                return Variant(Color(
                    (float)map_get_double(d, "r"),
                    (float)map_get_double(d, "g"),
                    (float)map_get_double(d, "b"),
                    (float)a));
            }
            if (is_vec2_map(d)) {
                return Variant(Vector2(
                    (float)map_get_double(d, "x"),
                    (float)map_get_double(d, "y")));
            }
            // Generic map — fall through to string
            MiniScript::String s = v.ToString(vm);
            return Variant(String::utf8(s.c_str()));
        }
        default: {
            MiniScript::String s = v.ToString(vm);
            return Variant(String::utf8(s.c_str()));
        }
    }
}

} // namespace MiniScriptBridge
