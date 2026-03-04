#pragma once

#include "core/object/script_language.h"
#include "core/templates/hash_map.h"

class MiniScriptInstance;

class MiniScript : public Script {
    GDCLASS(MiniScript, Script);

    struct ParsedEmitArgument {
        bool is_literal = false;
        Variant literal_value;
        String expression;
    };

    struct ParsedEmitAction {
        StringName signal_name;
        int source_line = -1;
        Vector<ParsedEmitArgument> arguments;
    };

    struct ParsedMethod {
        MethodInfo info;
        int declaration_line = -1;
        bool has_print = false;
        int print_line = -1;
        bool print_is_literal = false;
        Variant print_value;
        String print_expression;
        bool has_return = false;
        int return_line = -1;
        bool return_is_literal = false;
        Variant return_value;
        String return_expression;
        bool has_runtime_issue = false;
        int runtime_issue_line = -1;
        String runtime_issue_message;
        Vector<ParsedEmitAction> emit_actions;
    };

    struct ParsedProperty {
        PropertyInfo info;
        Variant default_value;
        bool has_default = false;
    };

    String source_code;
    StringName global_name;
    StringName base_type = SNAME("Object");
    HashMap<StringName, ParsedMethod> parsed_methods;
    HashMap<StringName, ParsedProperty> parsed_properties;
    HashMap<StringName, MethodInfo> parsed_signals;
    bool tool_mode = false;
    bool valid = true;

    void _parse_source();
    static bool _parse_literal(const String &p_text, Variant &r_value);

public:
    static void _bind_methods();

    bool can_instantiate() const override;
    Ref<Script> get_base_script() const override;
    StringName get_global_name() const override;
    bool inherits_script(const Ref<Script> &p_script) const override;
    StringName get_instance_base_type() const override;
    ScriptInstance *instance_create(Object *p_this) override;
    bool instance_has(const Object *p_this) const override;

    bool has_source_code() const override;
    void set_source_code(const String &p_code) override;
    String get_source_code() const override;
    Error reload(bool p_keep_state = false) override;
    Variant call_script_method(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error, const MiniScriptInstance *p_instance = nullptr) const;

    bool has_exported_property(const StringName &p_property) const;
    bool get_exported_property_default(const StringName &p_property, Variant &r_value) const;

#ifdef TOOLS_ENABLED
    StringName get_doc_class_name() const override;
    Vector<DocData::ClassDoc> get_documentation() const override;
    String get_class_icon_path() const override;
#endif

    bool has_method(const StringName &p_method) const override;
    MethodInfo get_method_info(const StringName &p_method) const override;
    bool is_tool() const override;
    bool is_valid() const override;
    bool is_abstract() const override;

    ScriptLanguage *get_language() const override;

    bool has_script_signal(const StringName &p_signal) const override;
    void get_script_signal_list(List<MethodInfo> *r_signals) const override;
    bool get_property_default_value(const StringName &p_property, Variant &r_value) const override;
    void get_script_method_list(List<MethodInfo> *p_list) const override;
    void get_script_property_list(List<PropertyInfo> *p_list) const override;

    const Variant get_rpc_config() const override;
};
