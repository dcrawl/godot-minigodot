#pragma once

#include "core/io/resource_saver.h"

class MiniScriptResourceSaver : public ResourceFormatSaver {
    GDSOFTCLASS(MiniScriptResourceSaver, ResourceFormatSaver);

public:
    Error save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags = 0) override;
    void get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const override;
    bool recognize(const Ref<Resource> &p_resource) const override;
};
