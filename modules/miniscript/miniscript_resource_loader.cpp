#include "miniscript_resource_loader.h"

#include "core/io/file_access.h"
#include "miniscript_script.h"

Ref<Resource> MiniScriptResourceLoader::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
    const String load_path = p_original_path.is_empty() ? p_path : p_original_path;

    Error err = OK;
    const String source = FileAccess::get_file_as_string(load_path, &err);
    if (err != OK) {
        if (r_error != nullptr) {
            *r_error = err;
        }
        return Ref<Resource>();
    }

    Ref<MiniScriptScript> script;
    script.instantiate();
    script->set_source_code(source);
    script->set_path(load_path);

    if (r_error != nullptr) {
        *r_error = OK;
    }

    return script;
}

void MiniScriptResourceLoader::get_recognized_extensions(List<String> *p_extensions) const {
    p_extensions->push_back("ms");
}

bool MiniScriptResourceLoader::handles_type(const String &p_type) const {
    return (p_type == "Script" || p_type == "MiniScript");
}

String MiniScriptResourceLoader::get_resource_type(const String &p_path) const {
    if (p_path.get_extension().to_lower() == "ms") {
        return "MiniScript";
    }
    return String();
}
