

#ifndef SCENE_SYSTEM_H
#define SCENE_SYSTEM_H

#include "nav.h"
#include "unit.h"
#include "prop.h"

#define SCENE_SYSTEM_PROPS_MAX 96
#define SCENE_SYSTEM_UNITS_MAX 48
#define SCENE_SYSTEM_CHUNKS_MAX 12
#define SCENE_SYSTEM_LIGHT_MAX 64

typedef struct
{
    float radius;
    float angle;
    Vec3 point;
    Vec3 forward;
    Vec3 color;
} Light;

typedef struct
{
    UnitType type;
    void (*onSpawn)(Unit* unit, int flags);
} UnitSpawnTable;

typedef struct
{
    PropType type;
    void (*onSpawn)(Prop* prop, int flags);
} PropSpawnTable;

typedef struct
{
    const UnitSpawnTable* unitSpawnTables;
    const PropSpawnTable* propSpawnTables;
} SpawnTable;

typedef struct
{
    int model;
    int texture;
    
    AABB bounds;
} Chunk;

struct Engine;

typedef struct
{
    struct Engine* engine;
    
    int skullCount;
    
    Chunk chunks[SCENE_SYSTEM_CHUNKS_MAX];
    int chunkCount;
    
    Light lights[SCENE_SYSTEM_LIGHT_MAX];
    int lightCount;
        
    Unit units[SCENE_SYSTEM_UNITS_MAX];
    Prop props[SCENE_SYSTEM_PROPS_MAX];
        
    const SpawnTable* handler;
    
} SceneSystem;

extern void SceneSystem_Init(SceneSystem* sceneSystem, struct Engine* engine, const SpawnTable* handler);

/* units */
extern Unit* SceneSystem_SpawnUnit(SceneSystem* world, UnitType type);
extern Unit* SceneSystem_SpawnUnitAt(SceneSystem* world, UnitType type, Vec3 position, float angle, int flags);
extern Unit* SceneSystem_FindUnitType(SceneSystem* world, UnitType type, int playerId);
extern void SceneSystem_Select(SceneSystem* world, int entityId);
extern void SceneSystem_ClearSelection(SceneSystem* world);

/* props */
extern Prop* SceneSystem_SpawnProp(SceneSystem* world, PropType type);
extern Prop* SceneSystem_SpawnPropAt(SceneSystem* world,
                                     PropType type,
                                     Vec3 position,
                                     Quat rotation,
                                     int flags);

extern Prop* SceneSystem_SpawnPropDetail(SceneSystem* world,
                                         PropType type,
                                         Vec3 position,
                                         Quat rotation,
                                         AABB bounds,
                                         Vec3 spawnEuler,
                                         int flags);

extern Prop* SceneSystem_FindProp(SceneSystem* world, const char* identifier);

extern void SceneSystem_Clear(SceneSystem* world);



#endif
