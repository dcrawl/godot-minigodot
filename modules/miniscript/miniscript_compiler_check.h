#pragma once

#include "core/object/script_language.h"
#include "core/string/ustring.h"
#include "core/templates/list.h"

// Attempts to compile pre-preprocessed MiniScript source.
// Returns true on success; appends any error to r_errors (if non-null).
// Compiled with C++ exceptions enabled (bridge source).
bool ms_compiler_check(
        const String &p_preprocessed_source,
        const String &p_path,
        List<ScriptLanguage::ScriptError> *r_errors);
