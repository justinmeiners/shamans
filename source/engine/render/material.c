
#include "material.h"
#include <assert.h>
#include <memory.h>

void Material_Init(Material* material)
{
    assert(material);
    material->flags = kMaterialFlagNone;
    material->diffuseMap = -1;
}

void Material_Copy(Material* dest, const Material* source)
{
    assert(dest && source);
    memcpy(dest, source, sizeof(Material));
}
