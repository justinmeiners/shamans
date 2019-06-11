

#ifndef PROP_H
#define PROP_H

#include "static_model.h"
#include "geo_math.h"
#include "nav_mesh.h"

typedef enum
{
    kPropNone = 0,
    kPropUnitSpawn,
    kPropSkull,
    kPropWeapon,
    kPropMgBullet,
    kPropRevolverBullet,
    kPropCannonBullet,
    kPropMeleeDamage,
    kPropMagicBall,
    kPropVampBall,
    kPropMindDamage,
    kPropWeaponPickup,
    kPropItemPickup,
    kPropButton,
    kPropTrigger,
    kPropEggHealer,
    kPropEyeMine,
    kPropAcidDamage,
} PropType;

typedef enum
{
    kDamageTypeOther = 0,
    kDamageTypeMelee,
    kDamageTypeBullet,
    kDamageTypeMagic,
    kDamageTypeAcid,
    kDamageTypeMindControl,
    kDamageTypeHeal
} DamageType;


typedef enum
{
    kEventNone = -1,
    kEventSpawn = 0,
    kEventDie,
    kEventActivate,
    kEventDeactivate,
    kEventDamage,
    kEventAction,
    kEventStartTurn,
    kEventCount
} EventType;

#define PROP_IDENTIFIER_MAX 64
#define PROP_EVENTS_MAX 8
#define PROP_DATA_VAR_MAX 8

typedef struct
{
    EventType event;
    int data;
    
    /* tag of entity to find */
    char target[PROP_IDENTIFIER_MAX];
    
    // the event we want to trigger in the target 
    EventType action;
    
} PropEventTrigger;

struct Unit;


typedef struct Prop
{
    struct Engine* engine;

    int index;
    PropType type;
    
    struct Unit* owner;
    
    NavPoly* navPoly;
    char identifier[PROP_IDENTIFIER_MAX];
    
    int dead;
    int inactive;
    int touchEnabled;

    int visible;
    StaticModel model;
    
    int hp;
    
    Vec3 position;
    Vec3 target;
    Vec3 forward;
    Quat rotation;
    
    Mat4 worldMatrix;
    
    /* for retrieving angle placed in level editor */
    Vec3 spawnRotation; 
    
    AABB bounds;
    
    int playerId;
    
    int timer;
    
    float speed;
    int data[PROP_DATA_VAR_MAX];
    
    void (*onSpawn)(struct Prop* prop, int flags);
    void (*onTouchUnit)(struct Prop* prop, struct Unit* unit);
    void (*onTouch)(struct Prop* prop, struct Prop* other);
    
    void (*onDamage)(struct Prop* prop,
                     struct Prop* sender,
                     int damage,
                     DamageType type);
    
    void (*onAction)(struct Prop* prop, struct Unit* sender);
    void (*onTick)(struct Prop* prop);
    void (*onEvent)(struct Prop* prop, struct Prop* sender, EventType event, int data);
    void (*onVar)(struct Prop* prop, const char* key, const char* var);
    
    PropEventTrigger events[PROP_EVENTS_MAX];
    
} Prop;

extern int Prop_Init(Prop* prop, struct Engine* engine, int index);

extern void Prop_SendVar(Prop* target, const char* key, const char* val);

extern void Prop_InputEvent(Prop* target, Prop* sender, EventType event, int data);
extern void Prop_OutputEvent(Prop* target, Prop* sender, EventType event, int data);

extern void Prop_Kill(Prop* prop);

extern void Prop_Damage(Prop* prop,
                        Prop* other,
                        int damage,
                        DamageType damageType);

#endif
