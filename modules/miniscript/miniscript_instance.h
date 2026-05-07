#pragma once

#include "core/object/script_instance.h"

class MiniScriptScript;
class MiniScriptLanguage;

class MiniScriptInstance : public ScriptInstance {
    Ref<MiniScriptScript> script;
    Object *owner = nullptr;
    HashMap<StringName, Variant> property_values;

    // Real MiniScript interpreter for this instance (lazy-initialized).
    // Stored as void* to avoid including MiniScript headers in this public header
    // (MiniScript is both a Godot class name and the thirdparty interpreter namespace).
    void *ms_interp = nullptr;
    bool ms_interp_ready = false;

    void _ensure_interpreter();
    void _sync_props_to_interp();
    void _sync_props_from_interp();

public:
    void _reset_interpreter();
    // Snapshots live locals from the interpreter's top call frame.
    // Safe to call from a debug-poll callback while the interpreter is mid-execution.
    void snapshot_live_locals(Vector<String> &r_names, Vector<Variant> &r_values) const;
    void snapshot_exported_properties(Vector<String> &r_names, Vector<Variant> &r_values) const;

    MiniScriptInstance(const Ref<MiniScriptScript> &p_script, Object *p_owner);
    ~MiniScriptInstance();

    Object *get_mini_owner() const;
    bool get_mini_property_value(const StringName &p_name, Variant &r_value) const;

    bool set(const StringName &p_name, const Variant &p_value) override;
    bool get(const StringName &p_name, Variant &r_ret) const override;
    void get_property_list(List<PropertyInfo> *p_properties) const override;
    Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;
    void validate_property(PropertyInfo &p_property) const override;
    bool property_can_revert(const StringName &p_name) const override;
    bool property_get_revert(const StringName &p_name, Variant &r_ret) const override;
    void get_method_list(List<MethodInfo> *p_list) const override;
    bool has_method(const StringName &p_method) const override;
    Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
    void notification(int p_notification, bool p_reversed = false) override;

    Object *get_owner() override;
    Ref<Script> get_script() const override;
    ScriptLanguage *get_language() override;
    const Variant get_rpc_config() const override;
};
