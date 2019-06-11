
#include "scene_system.h"
#include "platform.h"
#include "engine.h"

static void Chunk_Init(Chunk* chunk)
{
    chunk->model = -1;
    chunk->texture = -1;
}

void SceneSystem_Init(SceneSystem* world, struct Engine* engine, const SpawnTable* handler)
{
    world->engine = engine;
    world->chunkCount = 0;
    world->lightCount = 0;
    
    for (int i = 0; i < SCENE_SYSTEM_CHUNKS_MAX; ++i)
        Chunk_Init(world->chunks + i);
    
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
        Unit_Init(world->units + i, engine, i);
    
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
        Prop_Init(world->props + i, engine, i);
    
    world->skullCount = 3;
    world->handler = handler;
}


Unit* SceneSystem_SpawnUnitAt(SceneSystem* world, UnitType type, Vec3 point, float angle, int flags)
{
    /* see if we recognize this entity type */
    const UnitSpawnTable* j = world->handler->unitSpawnTables;
    
    if (type != 0)
    {
        while (j->type != -1)
        {
            if (j->type == type)
            {
                break;
            }
            
            j++;
        }
        
        if (j->type == -1) { return NULL; }
    }
    
    /* find an empty slot for entities */
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        Unit* unit = world->units + i;
        
        if (unit->dead)
        {
            Unit_Init(unit, world->engine, i);
            unit->dead = 0;
            unit->type = type;
            
            unit->onSpawn = j->onSpawn;
            
            unit->position = point;
            unit->angle = angle;
            unit->targetAngle = unit->angle;
            
            if (unit->onSpawn)
                unit->onSpawn(unit, flags);
            
            return unit;
        }
    }
    
    return NULL;
}

Unit* SceneSystem_SpawnUnit(SceneSystem* world, UnitType type)
{
    return SceneSystem_SpawnUnitAt(world, type, Vec3_Zero, 0.0f, 0);
}

Unit* SceneSystem_FindUnitType(SceneSystem* world, UnitType type, int playerId)
{
    for (int j = 0; j < SCENE_SYSTEM_UNITS_MAX; ++j)
    {
        const Unit* unit = world->units + j;
        
        if (unit->state == kUnitStateDead || unit->dead || unit->playerId != playerId) { continue; }
        if (type != unit->type && type != -1) { continue; }
        
        return world->units + j;
    }
    
    return NULL;
}

Prop* SceneSystem_SpawnPropAt(SceneSystem* world,
                              PropType type,
                              Vec3 position,
                              Quat rotation,
                              int flags)
{
    
    AABB defaultBounds = AABB_CreateCentered(position, Vec3_Create(1.5f, 1.5f, 1.5f));
    return SceneSystem_SpawnPropDetail(world, type, position, rotation, defaultBounds, Vec3_Zero, flags);
}


Prop* SceneSystem_SpawnPropDetail(SceneSystem* world,
                                  PropType type,
                                  Vec3 position,
                                  Quat rotation,
                                  AABB bounds,
                                  Vec3 spawnEuler,
                                  int flags)
{
    /* see if we recognize this entity type */
    const PropSpawnTable* j = world->handler->propSpawnTables;
    
    if (type != 0)
    {
        while (j->type != kPropNone)
        {
            if (j->type == type)
                break;
            ++j;
        }
        
        if (j->type == kPropNone)
            return NULL;
    }
    
    // find an empty slot for entities
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
    {
        Prop* prop = world->props + i;
        
        if (prop->dead)
        {
            Prop_Init(prop, world->engine, i);
            prop->dead = 0;
            prop->type = type;
            
            prop->onSpawn = j->onSpawn;
            
            prop->position = position;
            prop->rotation = rotation;
            prop->bounds = bounds;
            prop->spawnRotation = spawnEuler;
            
            if (prop->onSpawn)
                prop->onSpawn(prop, flags);
            
            Prop_OutputEvent(prop, NULL, kEventSpawn, 0);
            
            return prop;
        }
    }
    
    return NULL;
}

Prop* SceneSystem_SpawnProp(SceneSystem* world, PropType type)
{
    return SceneSystem_SpawnPropAt(world, type, Vec3_Zero, Quat_Identity, 0);
}

Prop* SceneSystem_FindProp(SceneSystem* world, const char* identifier)
{
    for (int j = 0; j < SCENE_SYSTEM_PROPS_MAX; ++j)
    {
        if (world->props[j].dead) continue;
        
        if (strcmp(world->props[j].identifier, identifier) == 0)
            return world->props + j;
    }
    
    return NULL;
}

void SceneSystem_Clear(SceneSystem* world)
{
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
        world->units[i].dead = 1;
    
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
        world->props[i].dead = 1;

    world->chunkCount = 0;
}


