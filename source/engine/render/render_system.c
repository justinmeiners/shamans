
#include "render_system.h"
#include "platform.h"
#include "Engine.h"

typedef struct
{
    float distSq;
    int lightIndex;
    
} LightSearch;

static int LightSearch_Compare(const void* a, const void* b)
{
    const LightSearch* la = a;
    const LightSearch* lb = b;
    return (la->distSq < lb->distSq) ? -1 : 1;
}


int RenderSystem_Init(RenderSystem* system,
                      struct Engine* engine,
                      Renderer* renderer,
                      short renderWidth,
                      short renderHeight)
{
    if (system && renderer)
    {
        HintBuffer_Init(&system->hintBuffer);
        Frustum_Init(&system->cam, 45.0f, 1.0f, 50.0f);
        
        system->camOffset = Vec3_Create(3.0f, -36.0f, 36.0f);

        system->cam.position = system->camOffset;
        system->cam.target = Vec3_Zero;
        system->cam.orientation = Vec3_Create(0.0f, 0.0f, 1.0f);
        system->cam.near = 15.0f;
        system->cam.far = 110.0f;
        
        system->viewportWidth = renderWidth;
        system->viewportHeight = renderHeight;

        system->scaleFactor = 1.0f;
                
        system->renderer = renderer;
        
        system->renderer->prepareGuiBuffer(renderer, &engine->guiSystem.buffer);
        system->renderer->prepareHintBuffer(renderer, &engine->renderSystem.hintBuffer);
        
        return 1;
    }
    
    return 0;
}

void RenderSystem_Shutdown(RenderSystem* system, struct Engine* engine)
{
    system->renderer->shutdown(system->renderer);

    system->renderer->cleanupGuiBuffer(system->renderer, &engine->guiSystem.buffer);
    system->renderer->cleanupHintBuffer(system->renderer, &engine->renderSystem.hintBuffer);
}

int RenderSystem_LoadTexture(RenderSystem* system, int texture, TextureFlags flags, const char* path)
{
    Texture* tex = system->textures + texture;
    
    if (path == NULL)
    {
        system->renderer->cleanupTexture(system->renderer, tex);
        Texture_Shutdown(tex);
        return 0;
    }
    
    char fullPath[MAX_OS_PATH];
    Filepath_Append(fullPath, Filepath_DataDir(), path);
    
    if (Texture_FromPath(tex, flags, fullPath))
    {
        if (tex->width > system->renderer->limits.maxTextureSize)
        {
            printf("invalid texture size\n");
            return 0;
        }
        return system->renderer->uploadTexture(system->renderer, tex);
    }
    else
    {
        return 0;
    }
}

int RenderSystem_LoadStaticModel(RenderSystem* system, int modelIndex, const char* path)
{
    StaticModel* model = system->models + modelIndex;

    if (path == NULL)
    {
        system->renderer->cleanupMesh(system->renderer, &model->mesh);
        StaticModel_Shutdown(model);
        return 0;
    }
    
    char fullPath[MAX_OS_PATH];
    Filepath_Append(fullPath, Filepath_DataDir(), path);
    
    if (StaticModel_FromPath(model, fullPath))
    {
        return system->renderer->uploadMesh(system->renderer, &model->mesh);
    }
    else
    {
        return 0;
    }
}

int RenderSystem_LoadAnim(RenderSystem* system, int animIndex, const char* path)
{
    SkelAnim* anim = system->anims + animIndex;
    
    if (path == NULL)
    {
        SkelAnim_Shutdown(anim);
        return 0;
    }
    
    char fullPath[MAX_OS_PATH];
    Filepath_Append(fullPath, Filepath_DataDir(), path);
    
    if (SkelAnim_FromPath(anim, fullPath))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int RenderSystem_LoadSkelModel(RenderSystem* system, int modelIndex, const char* path)
{
    SkelModel* model = system->skelModels + modelIndex;
    
    if (path == NULL)
    {
        system->renderer->cleanupSkelSkin(system->renderer, &model->skin);
        SkelModel_Shutdown(model);
        return 0;
    }
    
    char fullPath[MAX_OS_PATH];
    Filepath_Append(fullPath, Filepath_DataDir(), path);
    
    if (SkelModel_FromPath(model, fullPath))
    {
        if (model->skel.jointCount > system->renderer->limits.maxSkelJoints)
        {
            printf("invalid skeleton size\n");
            return 0;
        }
        
        return system->renderer->uploadSkelSkin(system->renderer, &model->skin);
    }
    else
    {
        return 0;
    }
}

static void RenderSystem_PrepareObservers(Renderer* renderer,
                                          const Frustum* cam,
                                          const FogView* vis,
                                          const Engine* engine,
                                          RenderList* renderList)
{
    short observerCount = 0;
    
    
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        const Unit* unit = engine->sceneSystem.units + i;
        
        if (!unit->dead && unit->playerId == ENGINE_PLAYER_LOCAL)
        {
            /* visbility data for shader */
            Sphere sphere = Sphere_Create(unit->position, unit->viewRadius);
            
            if (Sphere_IntersectsPoint(sphere, cam->position) || Frustum_SphereVisible(cam, sphere))
            {
                renderList->observerList.points[observerCount] = unit->position;
                renderList->observerList.radiiSq[observerCount] = unit->viewRadius * unit->viewRadius;
                
                ++observerCount;
                
                if (observerCount == OBSERVERS_MAX) break;
            }
        }
    }
    
    renderList->observerList.observerCount = observerCount;
}

static void RenderSystem_Cull(Renderer* renderer,
                              const Frustum* cam,
                              const FogView* playerView,
                              const Engine* engine,
                              RenderList* renderList)
{
    LightSearch lightSearchList[SCENE_SYSTEM_LIGHT_MAX];
    
    /* chunks */
    int counter = 0;
    for (int i = 0; i < engine->sceneSystem.chunkCount; ++i)
    {
        const Chunk* chunk = engine->sceneSystem.chunks + i;
        
        if (Frustum_AabbVisible(cam, chunk->bounds))
        {
            renderList->chunks[counter] = i;
            ++counter;
        }
    }
    
    renderList->chunkCount = counter;
    
    /* Props */
    counter = 0;
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
    {
        const Prop* prop = engine->sceneSystem.props + i;
        
        if (prop->dead) continue;
        if (!prop->model.loaded  || !prop->visible) continue;
        if (playerView->propVisibility[i] < 0.22f) continue;
        if (!Frustum_AabbVisible(cam, prop->bounds)) continue;
        
        renderList->props[counter] = i;
        
        int canidateCount = 0;
        if (!(prop->model.material.flags & kMaterialFlagUnlit))
        {
            for (int j = 0; j < engine->sceneSystem.lightCount; ++j)
            {
                const Light* light = engine->sceneSystem.lights + j;
                Vec3 dir = Vec3_Sub(prop->position, light->point);
                
                float bias = 5.0f;
                float radius = light->radius + bias;
                
                float angleBias = M_PI / 12.0f;
                
                if (Vec3_LengthSq(dir) < radius * radius &&
                    Vec3_Dot(light->forward, Vec3_Norm(dir)) > cosf(light->angle + angleBias))
                {
                    lightSearchList[canidateCount].lightIndex = j;
                    lightSearchList[canidateCount].distSq = Vec3_DistSq(prop->position, light->point);
                    
                    ++canidateCount;
                }
            }
            
            qsort(lightSearchList, canidateCount, sizeof(LightSearch), LightSearch_Compare);

        }
        
        for (int j = 0; j < LIGHTS_PER_OBJECT; ++j)
        {
            if (j >= canidateCount)
            {
                renderList->propLights[counter].lights[j] = -1;
                continue;
            }
            
            renderList->propLights[counter].lights[j] = lightSearchList[j].lightIndex;
        }
        
        ++counter;
    }
    
    renderList->propCount = counter;
    
    /* Units */
    counter = 0;
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        const Unit* unit = engine->sceneSystem.units + i;
        if (unit->dead) continue;
        if (playerView->unitVisibility[i] < 0.22) continue;
        if (!Frustum_AabbVisible(cam, unit->bounds)) continue;
        
        renderList->units[counter] = i;
        
        int canidateCount = 0;
        for (int j = 0; j < engine->sceneSystem.lightCount; ++j)
        {
            const Light* light = engine->sceneSystem.lights + j;
            Vec3 dir = Vec3_Sub(unit->position, light->point);
            
            float bias = 5.0f;
            float radius = light->radius + bias;
            
            float angleBias = M_PI / 12.0f;
            
            if (Vec3_LengthSq(dir) < radius * radius &&
                Vec3_Dot(light->forward, Vec3_Norm(dir)) > cosf(light->angle + angleBias))
            {
                lightSearchList[canidateCount].lightIndex = j;
                lightSearchList[canidateCount].distSq = Vec3_DistSq(unit->position, light->point);
                
                ++canidateCount;
            }
        }
        
        qsort(lightSearchList, canidateCount, sizeof(LightSearch), LightSearch_Compare);

        for (int j = 0; j < LIGHTS_PER_OBJECT; ++j)
        {
            if (j >= canidateCount)
            {
                renderList->unitLights[counter].lights[j] = -1;
                continue;
            }
            
            renderList->unitLights[counter].lights[j] = lightSearchList[j].lightIndex;
        }
        
        ++counter;
    }
    
    renderList->unitCount = counter;
    
    /* Part Emitters */
    counter = 0;
    for (int i = 0; i < PART_SYSTEM_EMITTERS_MAX; ++i)
    {
        const PartEmitter* emitter = engine->partSystem.emitters + i;
        
        Sphere bounds = Sphere_Create(emitter->position, emitter->boundsRadius);
        
        if (emitter->effect != kPartEffectNone &&
            emitter->playing &&
            emitter->partCount > 0 &&
            Frustum_SphereVisible(cam, bounds))
        {
            renderList->emitters[counter] = i;
            ++counter;
        }
    }
    
    renderList->emitterCount = counter;
}

void RenderSystem_Render(RenderSystem* system,
                         const Frustum* cam,
                         const struct Engine* engine)
{
    
    RenderList list;
    memset(&list, 0, sizeof(RenderList));
    
    RenderSystem_Cull(system->renderer, cam, &engine->fogView, engine, &list);
    RenderSystem_PrepareObservers(system->renderer, cam, &engine->fogView, engine, &list);
    
    system->renderer->render(system->renderer, engine, cam, &list);
}


