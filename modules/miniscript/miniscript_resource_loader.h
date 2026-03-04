#pragma once

#include "core/io/resource_loader.h"

class MiniScriptResourceLoader : public ResourceFormatLoader {
    GDSOFTCLASS(MiniScriptResourceLoader, ResourceFormatLoader);

public:
    Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
    void get_recognized_extensions(List<String> *p_extensions) const override;
    bool handles_type(const String &p_type) const override;
    String get_resource_type(const String &p_path) const override;
};
