#include "miniscript_script.h"

#include "core/debugger/engine_debugger.h"
#include "core/debugger/script_debugger.h"
#include "core/string/print_string.h"
#include "miniscript_instance.h"
#include "miniscript_language.h"

void MiniScript::_bind_methods() {
}

bool MiniScript::can_instantiate() const {
    return true;
}

Ref<Script> MiniScript::get_base_script() const {
    return Ref<Script>();
}

StringName MiniScript::get_global_name() const {
    return global_name;
}

bool MiniScript::inherits_script(const Ref<Script> &p_script) const {
    return p_script.ptr() == this;
}

StringName MiniScript::get_instance_base_type() const {
    return base_type;
}

ScriptInstance *MiniScript::instance_create(Object *p_this) {
    Ref<MiniScript> self(this);
    return memnew(MiniScriptInstance(self, p_this));
}

bool MiniScript::instance_has(const Object *p_this) const {
    return false;
}

bool MiniScript::has_source_code() const {
    return true;
}

void MiniScript::set_source_code(const String &p_code) {
    source_code = p_code;
    _parse_source();
}

String MiniScript::get_source_code() const {
    return source_code;
}

Error MiniScript::reload(bool p_keep_state) {
    _parse_source();
    return OK;
}

Variant MiniScript::call_script_method(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error, const MiniScriptInstance *p_instance) const {
    MiniScriptLanguage::clear_runtime_error();

    const String script_path = get_path().is_empty() ? String("<memory>") : get_path();
    const ParsedMethod *method = parsed_methods.getptr(p_method);
    if (method == nullptr) {
        r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
        MiniScriptLanguage::set_runtime_error(script_path, -1, vformat("method '%s' not found", String(p_method)));
        return Variant();
    }

    ScriptDebugger *script_debugger = EngineDebugger::get_script_debugger();

    struct DebugFrameGuard {
        bool active = false;
        ScriptDebugger *script_debugger = nullptr;
        DebugFrameGuard(const String &p_path, const String &p_function, int p_line) {
            MiniScriptLanguage::push_debug_frame(p_path, p_function, p_line);
            active = true;

            script_debugger = EngineDebugger::get_script_debugger();
            if (script_debugger != nullptr && script_debugger->get_lines_left() > 0 && script_debugger->get_depth() >= 0) {
                script_debugger->set_depth(script_debugger->get_depth() + 1);
            }
        }
        ~DebugFrameGuard() {
            if (script_debugger != nullptr && script_debugger->get_lines_left() > 0 && script_debugger->get_depth() >= 0) {
                script_debugger->set_depth(script_debugger->get_depth() - 1);
            }

            if (active) {
                MiniScriptLanguage::pop_debug_frame();
            }
        }
    } debug_frame_guard(script_path, String(p_method), method->declaration_line);

    auto poll_debug_line = [&](int p_line) -> bool {
        if (script_debugger == nullptr || !EngineDebugger::is_active()) {
            return false;
        }

        bool do_break = false;

        if (script_debugger->get_lines_left() > 0) {
            if (script_debugger->get_depth() <= 0) {
                script_debugger->set_lines_left(script_debugger->get_lines_left() - 1);
            }
            if (script_debugger->get_lines_left() <= 0) {
                do_break = true;
            }
        }

        if (!script_debugger->is_skipping_breakpoints() && script_debugger->is_breakpoint(p_line, script_path)) {
            do_break = true;
        }

        if (do_break) {
            MiniScriptLanguage::set_runtime_error(script_path, p_line, "breakpoint hit");
            MiniScriptLanguage::debug_break("Breakpoint", true, false);
        }

        EngineDebugger::get_singleton()->line_poll();
        return do_break;
    };

    if (script_debugger != nullptr) {
        if (!script_debugger->is_skipping_breakpoints() && script_debugger->is_breakpoint(method->declaration_line, script_path)) {
            const String hit_message = vformat("MiniScript breakpoint hit at %s:%d in function '%s'", script_path, method->declaration_line, String(p_method));
            print_line(hit_message);
            MiniScriptLanguage::set_runtime_error(script_path, method->declaration_line, "breakpoint hit");
            MiniScriptLanguage::debug_break("Breakpoint", true, false);
            r_error.error = Callable::CallError::CALL_OK;
            return Variant();
        }
    }

    if (method->has_runtime_issue) {
        r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
        MiniScriptLanguage::set_runtime_error(script_path, method->runtime_issue_line, method->runtime_issue_message);
        return Variant();
    }

    const int expected_args = method->info.arguments.size();
    if (p_argcount != expected_args) {
        r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
        r_error.argument = p_argcount;
        r_error.expected = expected_args;
        MiniScriptLanguage::set_runtime_error(script_path, method->declaration_line, vformat("method '%s' expected %d argument(s), got %d", String(p_method), expected_args, p_argcount));
        return Variant();
    }

    r_error.error = Callable::CallError::CALL_OK;

    if (method->has_print) {
        if (poll_debug_line(method->print_line >= 0 ? method->print_line : method->declaration_line)) {
            return Variant();
        }

        Variant print_value;
        if (method->print_is_literal) {
            print_value = method->print_value;
        } else {
            String expr = method->print_expression.strip_edges();
            Variant resolved;
            if (p_instance != nullptr && p_instance->get_mini_property_value(expr, resolved)) {
                print_value = resolved;
            } else {
                print_value = expr;
            }
        }
        print_line(print_value.stringify());
    }

    for (int emit_idx = 0; emit_idx < method->emit_actions.size(); emit_idx++) {
        const ParsedEmitAction &emit_action = method->emit_actions[emit_idx];

        if (poll_debug_line(emit_action.source_line >= 0 ? emit_action.source_line : method->declaration_line)) {
            return Variant();
        }

        const MethodInfo *signal_info = parsed_signals.getptr(emit_action.signal_name);
        if (signal_info == nullptr) {
            r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
            MiniScriptLanguage::set_runtime_error(script_path, emit_action.source_line, vformat("signal '%s' not found", String(emit_action.signal_name)));
            return Variant();
        }

        const int expected_signal_args = signal_info->arguments.size();
        const int actual_signal_args = emit_action.arguments.size();
        if (actual_signal_args != expected_signal_args) {
            r_error.error = actual_signal_args < expected_signal_args ? Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS : Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
            r_error.argument = actual_signal_args;
            r_error.expected = expected_signal_args;
            MiniScriptLanguage::set_runtime_error(script_path, emit_action.source_line, vformat("signal '%s' expected %d argument(s), got %d", String(emit_action.signal_name), expected_signal_args, actual_signal_args));
            return Variant();
        }

        Object *emit_owner = p_instance != nullptr ? p_instance->get_mini_owner() : nullptr;
        if (emit_owner == nullptr) {
            r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
            MiniScriptLanguage::set_runtime_error(script_path, emit_action.source_line, "cannot emit signal without script instance owner");
            return Variant();
        }

        Vector<Variant> emit_values;
        emit_values.resize(actual_signal_args);

        for (int arg_idx = 0; arg_idx < actual_signal_args; arg_idx++) {
            const ParsedEmitArgument &parsed_argument = emit_action.arguments[arg_idx];
            Variant resolved_argument;

            if (parsed_argument.is_literal) {
                resolved_argument = parsed_argument.literal_value;
            } else {
                String expr = parsed_argument.expression.strip_edges();
                Variant resolved_property;
                if (p_instance != nullptr && p_instance->get_mini_property_value(expr, resolved_property)) {
                    resolved_argument = resolved_property;
                } else {
                    resolved_argument = expr;
                }
            }

            emit_values.write[arg_idx] = resolved_argument;
        }

        Vector<const Variant *> emit_arg_ptrs;
        emit_arg_ptrs.resize(actual_signal_args);
        for (int arg_idx = 0; arg_idx < actual_signal_args; arg_idx++) {
            emit_arg_ptrs.write[arg_idx] = &emit_values[arg_idx];
        }

        const Variant **emit_args = actual_signal_args > 0 ? emit_arg_ptrs.ptrw() : nullptr;
        Error emit_error = emit_owner->emit_signalp(emit_action.signal_name, emit_args, actual_signal_args);

        if (emit_error != OK) {
            r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
            MiniScriptLanguage::set_runtime_error(script_path, emit_action.source_line, vformat("failed to emit signal '%s'", String(emit_action.signal_name)));
            return Variant();
        }
    }

    if (method->has_return) {
        if (poll_debug_line(method->return_line >= 0 ? method->return_line : method->declaration_line)) {
            return Variant();
        }

        if (method->return_is_literal) {
            return method->return_value;
        }

        String expr = method->return_expression.strip_edges();
        Variant resolved;
        if (p_instance != nullptr && p_instance->get_mini_property_value(expr, resolved)) {
            return resolved;
        }
        return expr;
    }

    return Variant();
}

#ifdef TOOLS_ENABLED
StringName MiniScript::get_doc_class_name() const {
    return StringName();
}

Vector<DocData::ClassDoc> MiniScript::get_documentation() const {
    return Vector<DocData::ClassDoc>();
}

String MiniScript::get_class_icon_path() const {
    return String();
}
#endif

bool MiniScript::has_method(const StringName &p_method) const {
    return parsed_methods.has(p_method);
}

MethodInfo MiniScript::get_method_info(const StringName &p_method) const {
    if (const ParsedMethod *method = parsed_methods.getptr(p_method)) {
        return method->info;
    }
    return MethodInfo();
}

bool MiniScript::is_tool() const {
    return tool_mode;
}

bool MiniScript::is_valid() const {
    return valid;
}

bool MiniScript::is_abstract() const {
    return false;
}

ScriptLanguage *MiniScript::get_language() const {
    return MiniScriptLanguage::get_singleton();
}

bool MiniScript::has_script_signal(const StringName &p_signal) const {
    return parsed_signals.has(p_signal);
}

void MiniScript::get_script_signal_list(List<MethodInfo> *r_signals) const {
    for (const KeyValue<StringName, MethodInfo> &entry : parsed_signals) {
        r_signals->push_back(entry.value);
    }
}

bool MiniScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
    return get_exported_property_default(p_property, r_value);
}

void MiniScript::get_script_method_list(List<MethodInfo> *p_list) const {
    for (const KeyValue<StringName, ParsedMethod> &entry : parsed_methods) {
        p_list->push_back(entry.value.info);
    }
}

void MiniScript::get_script_property_list(List<PropertyInfo> *p_list) const {
    for (const KeyValue<StringName, ParsedProperty> &entry : parsed_properties) {
        p_list->push_back(entry.value.info);
    }
}

const Variant MiniScript::get_rpc_config() const {
    return Variant();
}

bool MiniScript::_parse_literal(const String &p_text, Variant &r_value) {
    const String text = p_text.strip_edges();
    if (text.is_empty()) {
        return false;
    }

    if (text == "true") {
        r_value = true;
        return true;
    }
    if (text == "false") {
        r_value = false;
        return true;
    }
    if (text == "nil" || text == "null") {
        r_value = Variant();
        return true;
    }

    if (text.is_valid_int()) {
        r_value = text.to_int();
        return true;
    }
    if (text.is_valid_float()) {
        r_value = text.to_float();
        return true;
    }

    if (text.length() >= 2 && text.begins_with("\"") && text.ends_with("\"")) {
        r_value = text.substr(1, text.length() - 2);
        return true;
    }

    return false;
}

void MiniScript::_parse_source() {
    global_name = StringName();
    base_type = SNAME("Object");
    parsed_methods.clear();
    parsed_properties.clear();
    parsed_signals.clear();
    tool_mode = false;

    if (source_code.is_empty()) {
        return;
    }

    PackedStringArray lines = source_code.split("\n", false);
    const int line_count = lines.size();

    bool in_function = false;
    for (int i = 0; i < line_count; i++) {
        String line = lines[i].strip_edges();
        String lower = line.to_lower();

        if (lower.begins_with("function ")) {
            in_function = true;
            continue;
        }
        if (in_function && lower == "end function") {
            in_function = false;
            continue;
        }

        if (in_function) {
            continue;
        }

        if (line.begins_with("class_name ")) {
            String class_declaration = line.substr(11).strip_edges();
            if (!class_declaration.is_empty()) {
                PackedStringArray class_parts = class_declaration.split(" ", false);
                if (!class_parts.is_empty()) {
                    global_name = class_parts[0].strip_edges();
                }
            }
            continue;
        }

        if (line.begins_with("extends ")) {
            String extends_declaration = line.substr(8).strip_edges();
            if (!extends_declaration.is_empty()) {
                PackedStringArray extends_parts = extends_declaration.split(" ", false);
                if (!extends_parts.is_empty()) {
                    base_type = extends_parts[0].strip_edges();
                }
            }
            continue;
        }

        if (lower == "tool" || lower == "@tool") {
            tool_mode = true;
            continue;
        }

        if (line.begins_with("signal ")) {
            String declaration = line.substr(7).strip_edges();
            if (declaration.is_empty()) {
                continue;
            }

            String signal_name = declaration;
            MethodInfo signal_info;

            int open_paren = declaration.find("(");
            int close_paren = declaration.rfind(")");
            if (open_paren >= 0 && close_paren > open_paren) {
                signal_name = declaration.substr(0, open_paren).strip_edges();
                String args_text = declaration.substr(open_paren + 1, close_paren - open_paren - 1).strip_edges();
                if (!args_text.is_empty()) {
                    PackedStringArray args = args_text.split(",", false);
                    for (int arg_idx = 0; arg_idx < args.size(); arg_idx++) {
                        String arg_name = args[arg_idx].strip_edges();
                        if (!arg_name.is_empty()) {
                            signal_info.arguments.push_back(PropertyInfo(Variant::NIL, arg_name));
                        }
                    }
                }
            }

            if (signal_name.is_empty()) {
                continue;
            }

            signal_info.name = signal_name;
            parsed_signals.insert(signal_name, signal_info);
            continue;
        }

        if (!line.begins_with("export ")) {
            continue;
        }

        String declaration = line.substr(7).strip_edges();
        if (declaration.is_empty()) {
            continue;
        }

        String property_name = declaration;
        String default_expr;
        int equals_pos = declaration.find("=");
        if (equals_pos >= 0) {
            property_name = declaration.substr(0, equals_pos).strip_edges();
            default_expr = declaration.substr(equals_pos + 1).strip_edges();
        }

        if (property_name.is_empty()) {
            continue;
        }

        ParsedProperty parsed_property;
        parsed_property.info.name = property_name;

        if (!default_expr.is_empty()) {
            Variant default_value;
            if (_parse_literal(default_expr, default_value)) {
                parsed_property.default_value = default_value;
                parsed_property.has_default = true;
                parsed_property.info.type = default_value.get_type();
            }
        }

        parsed_properties.insert(property_name, parsed_property);
    }

    for (int i = 0; i < line_count; i++) {
        String line = lines[i].strip_edges();
        if (!line.begins_with("function ")) {
            continue;
        }

        String signature = line.substr(9).strip_edges();
        if (signature.is_empty()) {
            continue;
        }

        String method_name = signature;
        ParsedMethod parsed;
        parsed.declaration_line = i + 1;

        int open_paren = signature.find("(");
        int close_paren = signature.rfind(")");
        if (open_paren >= 0 && close_paren > open_paren) {
            method_name = signature.substr(0, open_paren).strip_edges();
            String args_text = signature.substr(open_paren + 1, close_paren - open_paren - 1).strip_edges();
            if (!args_text.is_empty()) {
                PackedStringArray args = args_text.split(",", false);
                for (int arg_idx = 0; arg_idx < args.size(); arg_idx++) {
                    String arg_name = args[arg_idx].strip_edges();
                    if (!arg_name.is_empty()) {
                        parsed.info.arguments.push_back(PropertyInfo(Variant::NIL, arg_name));
                    }
                }
            }
        } else {
            PackedStringArray parts = signature.split(" ", false);
            if (!parts.is_empty()) {
                method_name = parts[0].strip_edges();
            }
        }

        if (method_name.is_empty()) {
            continue;
        }

        parsed.info.name = method_name;

        for (int j = i + 1; j < line_count; j++) {
            String body_line = lines[j].strip_edges();
            String lower = body_line.to_lower();

            if (lower == "end function") {
                i = j;
                break;
            }

            if (body_line.begins_with("print ")) {
                String print_expr = body_line.substr(6).strip_edges();
                parsed.print_line = j + 1;
                Variant parsed_print;
                if (_parse_literal(print_expr, parsed_print)) {
                    parsed.has_print = true;
                    parsed.print_is_literal = true;
                    parsed.print_value = parsed_print;
                } else {
                    parsed.has_print = true;
                    parsed.print_is_literal = false;
                    parsed.print_expression = print_expr;
                }
            } else if (body_line.begins_with("return ")) {
                String return_expr = body_line.substr(7).strip_edges();
                parsed.return_line = j + 1;
                Variant parsed_return;
                if (_parse_literal(return_expr, parsed_return)) {
                    parsed.has_return = true;
                    parsed.return_is_literal = true;
                    parsed.return_value = parsed_return;
                } else {
                    parsed.has_return = true;
                    parsed.return_is_literal = false;
                    parsed.return_expression = return_expr;
                }
            } else if (body_line.begins_with("emit ")) {
                String emit_expr = body_line.substr(5).strip_edges();
                if (emit_expr.is_empty()) {
                    parsed.has_runtime_issue = true;
                    parsed.runtime_issue_line = j + 1;
                    parsed.runtime_issue_message = "emit requires a signal name";
                    continue;
                }

                ParsedEmitAction emit_action;
                emit_action.source_line = j + 1;
                int split_pos = emit_expr.find(" ");
                if (split_pos >= 0) {
                    emit_action.signal_name = emit_expr.substr(0, split_pos).strip_edges();
                    String args_expr = emit_expr.substr(split_pos + 1).strip_edges();
                    if (!args_expr.is_empty()) {
                        PackedStringArray args = args_expr.split(",", false);
                        for (int arg_idx = 0; arg_idx < args.size(); arg_idx++) {
                            String arg_expr = args[arg_idx].strip_edges();
                            if (arg_expr.is_empty()) {
                                continue;
                            }

                            ParsedEmitArgument parsed_argument;
                            Variant parsed_arg;
                            if (_parse_literal(arg_expr, parsed_arg)) {
                                parsed_argument.is_literal = true;
                                parsed_argument.literal_value = parsed_arg;
                            } else {
                                parsed_argument.is_literal = false;
                                parsed_argument.expression = arg_expr;
                            }

                            emit_action.arguments.push_back(parsed_argument);
                        }
                    }
                } else {
                    emit_action.signal_name = emit_expr.strip_edges();
                }

                if (emit_action.signal_name.is_empty()) {
                    parsed.has_runtime_issue = true;
                    parsed.runtime_issue_line = j + 1;
                    parsed.runtime_issue_message = "emit requires a signal name";
                    continue;
                }

                parsed.emit_actions.push_back(emit_action);
            } else if (!body_line.is_empty() && !body_line.begins_with("#")) {
                parsed.has_runtime_issue = true;
                parsed.runtime_issue_line = j + 1;
                parsed.runtime_issue_message = vformat("unsupported statement '%s'", body_line);
            }

            if (j == line_count - 1) {
                i = j;
            }
        }

        parsed_methods.insert(method_name, parsed);
    }
}

bool MiniScript::has_exported_property(const StringName &p_property) const {
    return parsed_properties.has(p_property);
}

bool MiniScript::get_exported_property_default(const StringName &p_property, Variant &r_value) const {
    if (const ParsedProperty *property = parsed_properties.getptr(p_property)) {
        if (property->has_default) {
            r_value = property->default_value;
            return true;
        }
    }
    return false;
}
