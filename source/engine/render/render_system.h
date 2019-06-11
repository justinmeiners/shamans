
#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "renderer.h"
#include "texture.h"
#include "skel_model.h"
#include "static_model.h"
#include "hint.h"

#define RENDER_SYSTEM_MAX_MODELS 64
#define RENDER_SYSTEM_MAX_ANIMS 64


typedef struct
{    
    Vec3 camOffset;
    Frustum cam;
    
    short viewportWidth;
    short viewportHeight;
    
    float scaleFactor;
    
    Renderer* renderer;
    
    HintBuffer hintBuffer;
    
    StaticModel models[RENDER_SYSTEM_MAX_MODELS];
    SkelModel skelModels[RENDER_SYSTEM_MAX_MODELS];
    
    Texture textures[RENDER_SYSTEM_MAX_MODELS];
    SkelAnim anims[RENDER_SYSTEM_MAX_ANIMS];

} RenderSystem;


extern int RenderSystem_Init(RenderSystem* system,
                             struct Engine* engine,
                             Renderer* renderer,
                             short renderWidth,
                             short renderHeight);

extern void RenderSystem_Shutdown(RenderSystem* system, struct Engine* engine);


/* upload NULL for path to unload - replace operations are safetly handled */
extern int RenderSystem_LoadTexture(RenderSystem* system, int texture, TextureFlags flags, const char* path);
extern int RenderSystem_LoadStaticModel(RenderSystem* system, int model, const char* path);
extern int RenderSystem_LoadAnim(RenderSystem* system, int anim, const char* path);

extern int RenderSystem_LoadSkelModel(RenderSystem* system, int model, const char* path);

extern int RenderSystem_PrepareGuiBuffer(RenderSystem* system, GuiBuffer* buffer);
extern int RenderSystem_PrepareHintBuffer(RenderSystem* system, HintBuffer* buffer);

extern void RenderSystem_Render(RenderSystem* system,
                                const Frustum* cam,
                                const struct Engine* engine);
#endif
