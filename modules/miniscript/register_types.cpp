#include "register_types.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/os/memory.h"
#include "miniscript_godot_intrinsics.h"
#include "miniscript_language.h"
#include "miniscript_resource_loader.h"
#include "miniscript_resource_saver.h"
#include "miniscript_script.h"

static MiniScriptLanguage *miniscript_language = nullptr;
static Ref<MiniScriptResourceLoader> miniscript_resource_loader;
static Ref<MiniScriptResourceSaver> miniscript_resource_saver;

void initialize_miniscript_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
        return;
    }

    if (miniscript_language != nullptr) {
        return;
    }

    MiniScriptGodotIntrinsics::init_godot_intrinsics();

    GDREGISTER_CLASS(MiniScriptScript);

    miniscript_language = memnew(MiniScriptLanguage);
    ScriptServer::register_language(miniscript_language);

    miniscript_resource_loader.instantiate();
    ResourceLoader::add_resource_format_loader(miniscript_resource_loader);

    miniscript_resource_saver.instantiate();
    ResourceSaver::add_resource_format_saver(miniscript_resource_saver);
}

void uninitialize_miniscript_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
        return;
    }

    if (miniscript_resource_loader.is_valid()) {
        ResourceLoader::remove_resource_format_loader(miniscript_resource_loader);
        miniscript_resource_loader.unref();
    }

    if (miniscript_resource_saver.is_valid()) {
        ResourceSaver::remove_resource_format_saver(miniscript_resource_saver);
        miniscript_resource_saver.unref();
    }

    if (miniscript_language == nullptr) {
        return;
    }

    ScriptServer::unregister_language(miniscript_language);
    memdelete(miniscript_language);
    miniscript_language = nullptr;
}
