#include "miniscript_resource_saver.h"

#include "core/io/file_access.h"
#include "miniscript_script.h"

Error MiniScriptResourceSaver::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
    Ref<MiniScriptScript> script = p_resource;
    ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

    Error err = OK;
    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
    ERR_FAIL_COND_V_MSG(err != OK || file.is_null(), err == OK ? ERR_CANT_CREATE : err, "Cannot save MiniScript file '" + p_path + "'.");

    file->store_string(script->get_source_code());
    if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
        return ERR_CANT_CREATE;
    }

    return OK;
}

void MiniScriptResourceSaver::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
    if (Object::cast_to<MiniScriptScript>(*p_resource) != nullptr) {
        p_extensions->push_back("ms");
    }
}

bool MiniScriptResourceSaver::recognize(const Ref<Resource> &p_resource) const {
    return Object::cast_to<MiniScriptScript>(*p_resource) != nullptr;
}
