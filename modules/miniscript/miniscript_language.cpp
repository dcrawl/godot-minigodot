#include "miniscript_language.h"

#include "core/debugger/engine_debugger.h"
#include "core/debugger/script_debugger.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "miniscript_script.h"

MiniScriptLanguage *MiniScriptLanguage::singleton = nullptr;
thread_local String MiniScriptLanguage::debug_error;
thread_local String MiniScriptLanguage::debug_error_path;
thread_local int MiniScriptLanguage::debug_error_line = -1;
thread_local Vector<MiniScriptLanguage::DebugFrame> MiniScriptLanguage::debug_stack;
thread_local Vector<MiniScriptLanguage::DebugFrame> MiniScriptLanguage::debug_last_error_stack;

MiniScriptLanguage *MiniScriptLanguage::get_singleton() {
    return singleton;
}

void MiniScriptLanguage::clear_runtime_error() {
    debug_error = String();
    debug_error_path = String();
    debug_error_line = -1;
    debug_last_error_stack.clear();
}

void MiniScriptLanguage::set_runtime_error(const String &p_path, int p_line, const String &p_message) {
    debug_error_path = p_path;
    debug_error_line = p_line;
    debug_last_error_stack = debug_stack;

    String function_name;
    if (!debug_last_error_stack.is_empty()) {
        DebugFrame &frame = debug_last_error_stack.write[debug_last_error_stack.size() - 1];
        frame.source = p_path;
        if (p_line >= 0) {
            frame.line = p_line;
        }
        function_name = frame.function;
    }

    if (p_line >= 0) {
        if (!function_name.is_empty()) {
            debug_error = vformat("MiniScript runtime error at %s:%d in function '%s': %s", p_path, p_line, function_name, p_message);
        } else {
            debug_error = vformat("MiniScript runtime error at %s:%d: %s", p_path, p_line, p_message);
        }
    } else {
        if (!function_name.is_empty()) {
            debug_error = vformat("MiniScript runtime error at %s in function '%s': %s", p_path, function_name, p_message);
        } else {
            debug_error = vformat("MiniScript runtime error at %s: %s", p_path, p_message);
        }
    }
}

void MiniScriptLanguage::push_debug_frame(const String &p_path, const String &p_function, int p_line, const Vector<String> &p_local_names, const Vector<Variant> &p_local_values) {
    DebugFrame frame;
    frame.source = p_path;
    frame.function = p_function;
    frame.line = p_line;
    frame.local_names = p_local_names;
    frame.local_values = p_local_values;
    debug_stack.push_back(frame);
}

void MiniScriptLanguage::pop_debug_frame() {
    if (!debug_stack.is_empty()) {
        debug_stack.remove_at(debug_stack.size() - 1);
    }
}

void MiniScriptLanguage::update_debug_frame_line(int p_line) {
    if (!debug_stack.is_empty() && p_line >= 0) {
        debug_stack.write[debug_stack.size() - 1].line = p_line;
    }
}

bool MiniScriptLanguage::debug_break(const String &p_error, bool p_allow_continue, bool p_is_error_breakpoint) {
    if (!EngineDebugger::is_active() || singleton == nullptr) {
        return false;
    }

    if (Thread::get_caller_id() != Thread::get_main_id()) {
        return false;
    }

    const String env_toggle = OS::get_singleton()->get_environment("MINISCRIPT_DEBUG_BREAK").to_lower();
    const bool break_enabled = env_toggle == "1" || env_toggle == "true" || env_toggle == "yes" || env_toggle == "on";
    if (!break_enabled) {
        return false;
    }

    EngineDebugger::get_script_debugger()->debug(singleton, p_allow_continue, p_is_error_breakpoint);
    return true;
}

MiniScriptLanguage::MiniScriptLanguage() {
    singleton = this;
}

MiniScriptLanguage::~MiniScriptLanguage() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

String MiniScriptLanguage::get_name() const {
    return "MiniScript";
}

void MiniScriptLanguage::init() {
}

String MiniScriptLanguage::get_type() const {
    return "MiniScript";
}

String MiniScriptLanguage::get_extension() const {
    return "ms";
}

void MiniScriptLanguage::finish() {
}

Vector<String> MiniScriptLanguage::get_reserved_words() const {
    return Vector<String>();
}

bool MiniScriptLanguage::is_control_flow_keyword(const String &p_string) const {
    return false;
}

Vector<String> MiniScriptLanguage::get_comment_delimiters() const {
    Vector<String> delimiters;
    delimiters.push_back("#");
    return delimiters;
}

Vector<String> MiniScriptLanguage::get_doc_comment_delimiters() const {
    return Vector<String>();
}

Vector<String> MiniScriptLanguage::get_string_delimiters() const {
    Vector<String> delimiters;
    delimiters.push_back("\"");
    return delimiters;
}

bool MiniScriptLanguage::validate(const String &p_script, const String &p_path, List<String> *r_functions, List<ScriptError> *r_errors, List<Warning> *r_warnings, HashSet<int> *r_safe_lines) const {
    return true;
}

Script *MiniScriptLanguage::create_script() const {
    return memnew(MiniScriptScript);
}

bool MiniScriptLanguage::supports_builtin_mode() const {
    return true;
}

int MiniScriptLanguage::find_function(const String &p_function, const String &p_code) const {
    return -1;
}

String MiniScriptLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const {
    String source = "function ";
    source += p_name;
    source += "()\n";
    source += "\t# TODO: implement\n";
    source += "end function\n";
    return source;
}

void MiniScriptLanguage::auto_indent_code(String &p_code, int p_from_line, int p_to_line) const {
}

void MiniScriptLanguage::add_global_constant(const StringName &p_variable, const Variant &p_value) {
}

String MiniScriptLanguage::debug_get_error() const {
    return debug_error;
}

int MiniScriptLanguage::debug_get_stack_level_count() const {
    return debug_stack.is_empty() ? debug_last_error_stack.size() : debug_stack.size();
}

int MiniScriptLanguage::debug_get_stack_level_line(int p_level) const {
    const Vector<DebugFrame> &frames = debug_stack.is_empty() ? debug_last_error_stack : debug_stack;
    if (p_level >= 0 && p_level < frames.size()) {
        const int index = frames.size() - 1 - p_level;
        return frames[index].line;
    }
    return debug_error_line;
}

String MiniScriptLanguage::debug_get_stack_level_function(int p_level) const {
    const Vector<DebugFrame> &frames = debug_stack.is_empty() ? debug_last_error_stack : debug_stack;
    if (p_level >= 0 && p_level < frames.size()) {
        const int index = frames.size() - 1 - p_level;
        return frames[index].function;
    }
    return String();
}

String MiniScriptLanguage::debug_get_stack_level_source(int p_level) const {
    const Vector<DebugFrame> &frames = debug_stack.is_empty() ? debug_last_error_stack : debug_stack;
    if (p_level >= 0 && p_level < frames.size()) {
        const int index = frames.size() - 1 - p_level;
        return frames[index].source;
    }
    return debug_error_path;
}

void MiniScriptLanguage::debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {
    const Vector<DebugFrame> &frames = debug_stack.is_empty() ? debug_last_error_stack : debug_stack;
    if (p_level < 0 || p_level >= frames.size()) {
        return;
    }

    const int index = frames.size() - 1 - p_level;
    const DebugFrame &frame = frames[index];

    const int local_count = frame.local_names.size();
    for (int i = 0; i < local_count; i++) {
        p_locals->push_back(frame.local_names[i]);
        if (i < frame.local_values.size()) {
            p_values->push_back(frame.local_values[i]);
        } else {
            p_values->push_back(Variant());
        }
    }
}

void MiniScriptLanguage::debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {
}

void MiniScriptLanguage::debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {
}

String MiniScriptLanguage::debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) {
    const Vector<DebugFrame> &frames = debug_stack.is_empty() ? debug_last_error_stack : debug_stack;
    if (p_level < 0 || p_level >= frames.size()) {
        return String();
    }

    const String expression = p_expression.strip_edges();
    if (expression.is_empty()) {
        return String();
    }

    const int index = frames.size() - 1 - p_level;
    const DebugFrame &frame = frames[index];

    for (int i = 0; i < frame.local_names.size(); i++) {
        if (frame.local_names[i] == expression && i < frame.local_values.size()) {
            return frame.local_values[i].get_construct_string();
        }
    }

    return String();
}

Vector<ScriptLanguage::StackInfo> MiniScriptLanguage::debug_get_current_stack_info() {
    Vector<StackInfo> result;
    const int count = debug_stack.size();
    if (count == 0) {
        return result;
    }
    result.resize(count);
    for (int i = 0; i < count; i++) {
        const DebugFrame &frame = debug_stack[count - 1 - i];
        StackInfo &si = result.write[i];
        si.file = frame.source;
        si.func = frame.function;
        si.line = frame.line;
    }
    return result;
}

void MiniScriptLanguage::reload_all_scripts() {
}

void MiniScriptLanguage::reload_scripts(const Array &p_scripts, bool p_soft_reload) {
    for (int i = 0; i < p_scripts.size(); i++) {
        Variant script_variant = p_scripts[i];
        Ref<Script> script_ref = script_variant;
        if (script_ref.is_null()) {
            continue;
        }
        script_ref->reload(p_soft_reload);
    }
}

void MiniScriptLanguage::reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
    if (p_script.is_valid()) {
        p_script->reload(p_soft_reload);
    }
}

void MiniScriptLanguage::get_recognized_extensions(List<String> *p_extensions) const {
    p_extensions->push_back("ms");
}

void MiniScriptLanguage::get_public_functions(List<MethodInfo> *p_functions) const {
}

void MiniScriptLanguage::get_public_constants(List<Pair<String, Variant>> *p_constants) const {
}

void MiniScriptLanguage::get_public_annotations(List<MethodInfo> *p_annotations) const {
}

void MiniScriptLanguage::profiling_start() {
}

void MiniScriptLanguage::profiling_stop() {
}

void MiniScriptLanguage::profiling_set_save_native_calls(bool p_enable) {
}

int MiniScriptLanguage::profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) {
    return 0;
}

int MiniScriptLanguage::profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) {
    return 0;
}

bool MiniScriptLanguage::handles_global_class_type(const String &p_type) const {
    return p_type == "MiniScript";
}

String MiniScriptLanguage::get_global_class_name(const String &p_path, String *r_base_type, String *r_icon_path, bool *r_is_abstract, bool *r_is_tool) const {
    if (r_base_type != nullptr) {
        *r_base_type = "Object";
    }
    if (r_icon_path != nullptr) {
        *r_icon_path = String();
    }
    if (r_is_abstract != nullptr) {
        *r_is_abstract = false;
    }
    if (r_is_tool != nullptr) {
        *r_is_tool = false;
    }

    Error open_error = OK;
    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &open_error);
    if (open_error != OK || file.is_null()) {
        return String();
    }

    String class_name;
    PackedStringArray lines = file->get_as_utf8_string().split("\n", false);
    for (int i = 0; i < lines.size(); i++) {
        String line = lines[i].strip_edges();
        String lower = line.to_lower();

        if (line.begins_with("class_name ")) {
            String declaration = line.substr(11).strip_edges();
            if (!declaration.is_empty()) {
                PackedStringArray parts = declaration.split(" ", false);
                if (!parts.is_empty()) {
                    class_name = parts[0].strip_edges();
                }
            }
            continue;
        }

        if (line.begins_with("extends ") && r_base_type != nullptr) {
            String declaration = line.substr(8).strip_edges();
            if (!declaration.is_empty()) {
                PackedStringArray parts = declaration.split(" ", false);
                if (!parts.is_empty()) {
                    *r_base_type = parts[0].strip_edges();
                }
            }
            continue;
        }

        if ((lower == "tool" || lower == "@tool") && r_is_tool != nullptr) {
            *r_is_tool = true;
            continue;
        }
    }

    return class_name;
}
