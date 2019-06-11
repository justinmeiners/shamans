
#include "engine.h"
#include "platform.h"
#include <assert.h>

void Engine_LevelLoadPath(Engine* engine, const char* path)
{
    enum
    {
        DATA_NONE = 0,
        DATA_PROP,
        DATA_NAV,
        DATA_CHUNK,
        DATA_EVENT,
        DATA_VAR,
        DATA_ATLAS,
        DATA_SKULLS,
        DATA_LIGHT,
        DATA_VERSION,
    };
    
    char fullPath[MAX_OS_PATH];
    Filepath_Append(fullPath, Filepath_DataDir(), path);
    
    SceneSystem_Clear(&engine->sceneSystem);
    engine->sceneSystem.chunkCount = 0;
    
    Prop* lastProp = NULL;
    char buff[LINE_BUFFER_MAX];
    
    FILE* file = fopen(fullPath, "r");

    if (!file)
    {
        printf("Failed to open level: %s\n", fullPath);
        return;
    }
    
    while (!feof(file))
    {
        memset(buff, 0x0, LINE_BUFFER_MAX);
        fgets(buff, LINE_BUFFER_MAX, file);
        
        if (String_IsEmpty(buff))
            continue;
        
        char *savePtr = NULL;
        char* token = strtok_r(buff, ":", &savePtr);
        
        if (token == NULL) continue;

        int dataType = DATA_NONE;
        
        if (strcmp(token, "prop") == 0)
        {
            dataType = DATA_PROP;
        }
        else if (strcmp(token, "atlas") == 0)
        {
            dataType = DATA_ATLAS;
        }
        else if (strcmp(token, "nav") == 0)
        {
            dataType = DATA_NAV;
        }
        else if (strstr(token, "chunk") != NULL)
        {
            dataType = DATA_CHUNK;
        }
        else if (strstr(token, "skulls") != NULL)
        {
            dataType = DATA_SKULLS;
        }
        else if (strcmp(token, ".event") == 0)
        {
            dataType = DATA_EVENT;
        }
        else if (strcmp(token, ".var") == 0)
        {
            dataType = DATA_VAR;
        }
        else if (strcmp(token, "light") == 0)
        {
            dataType = DATA_LIGHT;
        }
        else if (strcmp(token, "version") == 0)
        {
            dataType = DATA_VERSION;
        }
        else
        {
            printf("%s\n", token);
            assert(0);
        }
        
        if (BUILD_DEBUG) printf("%s", token);
        
        token = strtok_r(NULL, ":", &savePtr);
        
        if (BUILD_DEBUG) printf("%s\n", token);
        
        switch (dataType)
        {
            case DATA_ATLAS:
            {
                char albedoPath[MAX_OS_PATH];
                sscanf(token, "%s", albedoPath);
                
                if (BUILD_DEBUG)
                    printf("loading atlas: %s\n", albedoPath);
                
                RenderSystem_LoadTexture(&engine->renderSystem, TEX_WORLD, kTextureFlagDefault | kTextureFlagAtlas, albedoPath);
                break;
            }
            case DATA_NAV:
            {
                char path[MAX_OS_PATH];
                sscanf(token, "%s", path);
                
                if (BUILD_DEBUG)
                    printf("loading nav: %s\n", path);
                
                NavSystem_LoadMesh(&engine->navSystem, path);
                break;
            }
            case DATA_CHUNK:
            {
                char path[MAX_OS_PATH];
                char lightmapPath[MAX_OS_PATH];
                
                Vec3 min, max;
                sscanf(token, "%s %s [%f, %f, %f] [%f, %f, %f]",
                       path,
                       lightmapPath,
                       &min.x, &min.y, &min.z,
                       &max.x, &max.y, &max.z);
                
                if (BUILD_DEBUG)
                    printf("loading chunk: %s\n%s\n", path, lightmapPath);
                
                RenderSystem_LoadStaticModel(&engine->renderSystem, MODEL_CHUNK0 + engine->sceneSystem.chunkCount, path);
                RenderSystem_LoadTexture(&engine->renderSystem, TEX_CHUNK0 + engine->sceneSystem.chunkCount, kTextureFlagDefault | kTextureFlagAtlas, lightmapPath);
                
                Chunk* chunk = engine->sceneSystem.chunks + engine->sceneSystem.chunkCount;
                
                chunk->model = MODEL_CHUNK0 + engine->sceneSystem.chunkCount;
                chunk->texture = TEX_CHUNK0 + engine->sceneSystem.chunkCount;
                chunk->bounds = AABB_Create(min, max);
                
                ++engine->sceneSystem.chunkCount;
                
                break;
            }
            case DATA_LIGHT:
            {
                Light* light = engine->sceneSystem.lights + engine->sceneSystem.lightCount;
                sscanf(token,
                       " %f %f [%f, %f, %f] [%f, %f, %f] [%f, %f, %f]",
                       &light->radius,
                       &light->angle,
                       &light->point.x,  &light->point.y, &light->point.z,
                       &light->forward.x, &light->forward.y, &light->forward.z,
                       &light->color.x, &light->color.y, &light->color.z);
                
                ++engine->sceneSystem.lightCount;
                break;
            }
            case DATA_PROP:
            {
                char typeName[PROP_IDENTIFIER_MAX];
                char identifier[PROP_IDENTIFIER_MAX];
                
                Vec3 min, max, point, euler;
                
                int flags;
                
                sscanf(token,
                       "%s %s %i [%f, %f, %f] [%f, %f, %f] [%f, %f, %f] [%f, %f, %f]",
                       typeName,
                       identifier,
                       &flags,
                       &point.x, &point.y, &point.z,
                       &euler.x, &euler.y, &euler.z,
                       &min.x, &min.y, &min.z,
                       &max.x, &max.y, &max.z);
                
                PropType type = kPropNone;
                
                const EnumTable* i = engine->levelLoadTable->propEnumTable;
                
                while (i->name != NULL)
                {
                    if (strcmp(i->name, typeName) == 0)
                    {
                        type = i->value;
                        break;
                    }
                    
                    ++i;
                }
                
                if (type != kPropNone)
                {
                    lastProp = SceneSystem_SpawnPropDetail(&engine->sceneSystem,
                                                           i->value,
                                                           point,
                                                           Quat_CreateEuler(euler.x, euler.y, euler.z),
                                                           AABB_Create(min, max),
                                                           euler,
                                                           flags);
                    if (lastProp)
                        strncpy(lastProp->identifier, identifier, PROP_IDENTIFIER_MAX);
                    
                }
                else
                {
                    printf("command failed: %s\n", typeName);
                }
                
                break;
            }
            case DATA_VAR:
            {
                assert(lastProp);
                
                char varName[64];
                char varValue[64];
                sscanf(token, "%s %s", varName, varValue);
                
                Prop_SendVar(lastProp, varName, varValue);
                break;
            }
            case DATA_EVENT:
            {
                assert(lastProp);
                
                char eventName[64];
                char actionName[64];
                
                int event = -1;
                int action = -1;
                
                char identifier[PROP_IDENTIFIER_MAX];
                int data;
                
                sscanf(token, "%s %s %s %i", eventName, identifier, actionName, &data);
                
                const EnumTable* j = engine->levelLoadTable->eventEnumTable;
                
                while (j->name != NULL)
                {
                    if (strcmp(j->name, eventName) == 0)
                    {
                        event = j->value;
                    }
                    if (strcmp(j->name, actionName) == 0)
                    {
                        action = j->value;
                    }
                    ++j;
                }
                
                if (event != -1 && action != -1)
                {
                    for (int i = 0; i < PROP_EVENTS_MAX; ++i)
                    {
                        if (lastProp->events[i].event == -1)
                        {
                            lastProp->events[i].event = event;
                            lastProp->events[i].action = action;
                            strcpy(lastProp->events[i].target, identifier);
                            lastProp->events[i].data = data;
                            
                            break;
                        }
                    }
                }
                else
                {
                    printf("could not interpret event: %s\n", token);
                }
                
                break;
            }
            case DATA_SKULLS:
            {
                int skullCount;
                sscanf(token, "%i", &skullCount);
                engine->sceneSystem.skullCount = skullCount;
                break;
            }
            case DATA_VERSION:
            {
                assert(atoi(token) == 2);
                break;
            }
            default:
                assert(0);
                break;
                
        }
    }

    fclose(file);
}


void Engine_UnloadLevel(Engine* engine)
{
    NavSystem_LoadMesh(&engine->navSystem, NULL);
    RenderSystem_LoadTexture(&engine->renderSystem, TEX_WORLD, kTextureFlagDefault | kTextureFlagAtlas, NULL);
    
    for (int i = 0; i < engine->sceneSystem.chunkCount; ++i)
    {
        RenderSystem_LoadStaticModel(&engine->renderSystem, MODEL_CHUNK0 + i, NULL);
        RenderSystem_LoadTexture(&engine->renderSystem, TEX_CHUNK0 + i, kTextureFlagDefault | kTextureFlagAtlas, NULL);
    }
}

