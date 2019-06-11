
#include "engine.h"


/*
 
 ASSETS
 
 TODO: add flags/options for import
 
 BACKGROUND
 --------------------------------------------
 I do not want a generalized Asset Manager for two reasons.
 1. C has no generic types.
 - so either messy void* features that sidestep C
 or specific functions and data structures for each asset type.
 2. AssetManager becomes a dependency of most all asset types.
 - Assets that load other files must register them with the assetmanager
 otherwise the loader becomes responsible
 - Assets must store a pointer to the asset manager for destruction or access through a singleton
 - Asset code cannot be used without asset manager.
 With a completely generic design in C++ it's likely these problems could be avoided, but creating a generic engine is not my goal.
 
 DESIGN
 --------------------------------------------
 1. lookup should use a tag id (int) instead of filename (string)
 - decoupling of resource names from paths and performance
 - assets need to be referencable by tag in the level file
 2. assets should be able to be dependent on other assets (materials, levels, models)
 3. file IO should only happen during designiated loading and not during game
 
 Perhaps this could be addresssed by having parent assets actually have ownership over their children
 - resources could not be shared between parents
 - how are indicies assigned for untagged assets?
 - how does the resource manager do automatic loads then?
 */


void Engine_UnloadAssets(Engine* engine)
{
    RenderSystem* gl = &engine->renderSystem;
    SndSystem* snd = &engine->soundSystem;
    
    for (int i = 0; i < Asset_textureCount; ++i)
    {
        const AssetEntry* entry = Asset_textureManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadTexture(gl, entry->identifier, kTextureFlagDefault, NULL);
        }
    }
    
    for (int i = 0; i < Asset_staticModelCount; ++i)
    {
        const AssetEntry* entry = Asset_staticModelManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadStaticModel(gl, entry->identifier, NULL);
        }
    }
    
    for (int i = 0; i < Asset_skelModelCount; ++i)
    {
        const AssetEntry* entry = Asset_skelModelManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadSkelModel(gl, entry->identifier, NULL);
        }
    }
    
    for (int i = 0; i < Asset_skelAnimCount; ++i)
    {
        const AssetEntry* entry = Asset_skelAnimManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadAnim(gl, entry->identifier, NULL);
        }
    }
    
    for (int i = 0; i < Asset_soundCount; ++i)
    {
        const AssetEntry* entry = Asset_soundManifest + i;
        
        if (entry->path)
        {
            SndSystem_LoadSound(snd, entry->identifier, NULL);
        }
    }
}


void Engine_LoadAssets(Engine* engine)
{
    RenderSystem* gl = &engine->renderSystem;
    SndSystem* snd = &engine->soundSystem;
        
    int i;
    for (i = 0; i < Asset_textureCount; ++i)
    {
        const AssetEntry* entry = Asset_textureManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadTexture(gl, entry->identifier, kTextureFlagDefault, entry->path);
        }
    }
    
    for (i = 0; i < Asset_staticModelCount; ++i)
    {
        const AssetEntry* entry = Asset_staticModelManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadStaticModel(gl, entry->identifier, entry->path);
        }
    }
    
    for (i = 0; i < Asset_skelModelCount; ++i)
    {
        const AssetEntry* entry = Asset_skelModelManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadSkelModel(gl, entry->identifier, entry->path);
        }
    }
    
    for (i = 0; i < Asset_skelAnimCount; ++i)
    {
        const AssetEntry* entry = Asset_skelAnimManifest + i;
        
        if (entry->path)
        {
            RenderSystem_LoadAnim(gl, entry->identifier, entry->path);
        }
    }

    for (i = 0; i < Asset_soundCount; ++i)
    {
        const AssetEntry* entry = Asset_soundManifest + i;
        
        if (entry->path)
        {
            SndSystem_LoadSound(snd, entry->identifier, entry->path);
        }
    }
}

