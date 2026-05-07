#include "miniscript_compiler_check.h"

// MiniScript headers — this file is compiled with C++ exceptions enabled.
#include "MiniscriptErrors.h"
#include "MiniscriptInterpreter.h"

bool ms_compiler_check(
        const String &p_preprocessed_source,
        const String &p_path,
        List<ScriptLanguage::ScriptError> *r_errors) {
    MiniScript::Interpreter interp;
    MiniScript::String src(p_preprocessed_source.utf8().get_data());

    try {
        interp.Reset(src);
        interp.Compile();
        return true;
    } catch (MiniScript::LexerException &e) {
        if (r_errors) {
            ScriptLanguage::ScriptError err;
            err.path = p_path;
            err.line = e.location.lineNum > 0 ? e.location.lineNum : 1;
            err.column = 1;
            err.message = String::utf8(e.message.c_str());
            r_errors->push_back(err);
        }
        return false;
    } catch (MiniScript::CompilerException &e) {
        if (r_errors) {
            ScriptLanguage::ScriptError err;
            err.path = p_path;
            err.line = e.location.lineNum > 0 ? e.location.lineNum : 1;
            err.column = 1;
            err.message = String::utf8(e.message.c_str());
            r_errors->push_back(err);
        }
        return false;
    } catch (MiniScript::MiniscriptException &e) {
        if (r_errors) {
            ScriptLanguage::ScriptError err;
            err.path = p_path;
            err.line = e.location.lineNum > 0 ? e.location.lineNum : 1;
            err.column = 1;
            err.message = String::utf8(e.message.c_str());
            r_errors->push_back(err);
        }
        return false;
    }
}
