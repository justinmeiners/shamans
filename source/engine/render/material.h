
#ifndef MATERIAL_H
#define MATERIAL_H

typedef enum
{
    kMaterialFlagNone = 0,
    kMaterialFlagUnlit = 1 << 0,    
} MaterialFlags;

typedef struct
{
    int diffuseMap;
    MaterialFlags flags;
} Material;

extern void Material_Init(Material* material);

extern void Material_Copy(Material* dest, const Material* source);

#endif
