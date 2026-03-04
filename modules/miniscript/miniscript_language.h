#pragma once

#include "core/object/script_language.h"

class MiniScriptLanguage : public ScriptLanguage {
    GDCLASS(MiniScriptLanguage, ScriptLanguage);

    struct DebugFrame {
        String source;
        String function;
        int line = -1;
    };

    static MiniScriptLanguage *singleton;
    static thread_local String debug_error;
    static thread_local String debug_error_path;
    static thread_local int debug_error_line;
    static thread_local Vector<DebugFrame> debug_stack;
    static thread_local Vector<DebugFrame> debug_last_error_stack;

public:
    static MiniScriptLanguage *get_singleton();
    static void clear_runtime_error();
    static void set_runtime_error(const String &p_path, int p_line, const String &p_message);
    static void push_debug_frame(const String &p_path, const String &p_function, int p_line);
    static void pop_debug_frame();
    static bool debug_break(const String &p_error, bool p_allow_continue = true, bool p_is_error_breakpoint = false);

    String get_name() const override;

    void init() override;
    String get_type() const override;
    String get_extension() const override;
    void finish() override;

    Vector<String> get_reserved_words() const override;
    bool is_control_flow_keyword(const String &p_string) const override;
    Vector<String> get_comment_delimiters() const override;
    Vector<String> get_doc_comment_delimiters() const override;
    Vector<String> get_string_delimiters() const override;
    bool validate(const String &p_script, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptError> *r_errors = nullptr, List<Warning> *r_warnings = nullptr, HashSet<int> *r_safe_lines = nullptr) const override;
    Script *create_script() const override;
    bool supports_builtin_mode() const override;
    int find_function(const String &p_function, const String &p_code) const override;
    String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const override;
    void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const override;
    void add_global_constant(const StringName &p_variable, const Variant &p_value) override;

    String debug_get_error() const override;
    int debug_get_stack_level_count() const override;
    int debug_get_stack_level_line(int p_level) const override;
    String debug_get_stack_level_function(int p_level) const override;
    String debug_get_stack_level_source(int p_level) const override;
    void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override;
    void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override;
    void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override;
    String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1) override;

    void reload_all_scripts() override;
    void reload_scripts(const Array &p_scripts, bool p_soft_reload) override;
    void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override;

    void get_recognized_extensions(List<String> *p_extensions) const override;
    void get_public_functions(List<MethodInfo> *p_functions) const override;
    void get_public_constants(List<Pair<String, Variant>> *p_constants) const override;
    void get_public_annotations(List<MethodInfo> *p_annotations) const override;

    void profiling_start() override;
    void profiling_stop() override;
    void profiling_set_save_native_calls(bool p_enable) override;
    int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) override;
    int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) override;

    bool handles_global_class_type(const String &p_type) const override;
    String get_global_class_name(const String &p_path, String *r_base_type = nullptr, String *r_icon_path = nullptr, bool *r_is_abstract = nullptr, bool *r_is_tool = nullptr) const override;

    MiniScriptLanguage();
    ~MiniScriptLanguage();
};
