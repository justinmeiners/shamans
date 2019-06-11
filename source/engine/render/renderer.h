
#ifndef RENDERER_H
#define RENDERER_H

#include "gui_system.h"
#include "scene_system.h"
#include "part_system.h"
#include "texture.h"
#include "hint.h"

/*
 
 GL_POINTS,
 GL_LINE_STRIP,
 GL_LINE_LOOP,
 GL_LINES,
 GL_TRIANGLE_STRIP,
 GL_TRIANGLE_FAN, and
 GL_TRIANGLES
 */

#define OBSERVERS_MAX 6
#define LIGHTS_PER_OBJECT 2


typedef struct
{
    int lights[LIGHTS_PER_OBJECT];
} LightEntry;

// structs below are for fog of war

typedef struct
{
    float unitVisibility[SCENE_SYSTEM_UNITS_MAX];
    float propVisibility[SCENE_SYSTEM_PROPS_MAX];
} FogView;


/* Render list allows culling, preprocessing, and sorting before rendering. */

typedef struct
{
    int units[SCENE_SYSTEM_UNITS_MAX];
    LightEntry unitLights[SCENE_SYSTEM_UNITS_MAX];
    int unitCount;
    
    int props[SCENE_SYSTEM_PROPS_MAX];
    LightEntry propLights[SCENE_SYSTEM_UNITS_MAX];
    int propCount;
    
    int chunks[SCENE_SYSTEM_CHUNKS_MAX];
    int chunkCount;
    
    int emitters[PART_SYSTEM_EMITTERS_MAX];
    int emitterCount;
    
    struct
    {
        Vec3 points[OBSERVERS_MAX];
        float radiiSq[OBSERVERS_MAX];
        short observerCount;
    } observerList;

} RenderList;

struct RenderSystem;


typedef struct
{
    unsigned short maxTextureSize;
    unsigned short maxSkelJoints;
} RendererLimits;

/* 
 The renderer completely abstracts rendering details from the rest of the game.
 This allows support of multiple OpenGL versions and ideally alternative graphics APIs.
 
 The Renderer needs to be aware of several systems including scene, gui, and particle data.
 */

typedef struct Renderer
{
    void (*shutdown)(struct Renderer* renderer);
    
    void (*render)(struct Renderer* renderer,
                   const struct Engine* engine,
                   const Frustum* cam,
                   const RenderList* list);
    
    int (*uploadTexture)(struct Renderer* renderer, Texture* texture);
    int (*uploadMesh)(struct Renderer* renderer, StaticMesh* mesh);
    int (*uploadSkelSkin)(struct Renderer* renderer, SkelSkin* skin);
    
    int (*cleanupTexture)(struct Renderer* renderer, Texture* texture);
    int (*cleanupMesh)(struct Renderer* renderer, StaticMesh* mesh);
    int (*cleanupSkelSkin)(struct Renderer* renderer, SkelSkin* skin);

    int (*prepareGuiBuffer)(struct Renderer* renderer, GuiBuffer* buffer);
    int (*prepareHintBuffer)(struct Renderer* renderer, HintBuffer* buffer);
    
    int (*cleanupGuiBuffer)(struct Renderer* renderer, GuiBuffer* buffer);
    int (*cleanupHintBuffer)(struct Renderer* renderer, HintBuffer* buffer);

    void (*beginLoading)(struct Renderer* renderer);
    void (*endLoading)(struct Renderer* renderer);
    
    RendererLimits limits;

    int debug;
    void* context;
    
} Renderer;


#endif
