
#ifndef UNIT_H
#define UNIT_H

#include "unit_info.h"
#include "nav.h"
#include "skel_model.h"
#include "prop.h"

typedef enum
{
    kUnitStateIdle = 0,
    kUnitStateMove,
    kUnitStatePreparePrimary,
    kUnitStateAttackPrimary,
    kUnitStatePrepareSecondary,
    kUnitStateAttackSecondary,
    kUnitStateTeleport,     // for witch/boss
    kUnitStateDead,
    kUnitStateHurt,
} UnitState;

struct Engine;

typedef struct Unit
{
    struct Engine* engine;
    
    int index;
    char name[UNIT_NAME_MAX];
    UnitType type;
    int dead;

    int playerId;
    int crewIndex;
    
    int maxHp;
    int hp;
    int previousTeleportHp;
    
    Vec3 position;
    Vec3 forward;
    
    float angle;
    float targetAngle;
    AABB bounds;
        
    struct Unit* target;
    const NavPoly* navPoly;
    
    NavPath path;
    int pathIndex;
    
    int doneForTurn;
    int actionCounter;
    
    float moveCounter;
    
    int moveRange;
    float radius;
    float speed;
    
    int viewRadius;
    int viewAngle;
    
    SkelModel skelModel;
    
    Prop* weaponProp;
    Prop* actionProp;
    
    short primaryWeapon;    
    short secondaryWeapon;
    
    int isAlerted;
    
    UnitState state;
    UnitPowerup powerups;
    
    int selected;
    
    int user1;
    int user2;
        
    void (*onSpawn)(struct Unit* unit, int flags);
    void (*onKill)(struct Unit* unit);
    
    void (*onTick)(struct Unit* unit);
    
    void (*onDamage)(struct Unit* unit,
                     struct Prop* sender,
                     int damage,
                     DamageType type);
    
    void (*onStartPath)(struct Unit* unit);
    
    void (*onStartTurn)(struct Unit* unit);
    void (*onSelect)(struct Unit* unit);
    void (*onDeslect)(struct Unit* unit);

} Unit;

extern void Unit_Init(Unit* unit, struct Engine* engine, int index);
extern void Unit_Kill(Unit* unit);

extern int Unit_StartPath(Unit* unit, Vec3 dest);
extern int Unit_FollowPath(Unit* unit);

extern void Unit_Damage(Unit* unit,
                        Prop* other,
                        int damage,
                        DamageType damageType);

extern void Unit_SetName(Unit* unit, const char* name);


extern int Unit_CanSee(const Unit* unit, const Unit* other, float range);

#endif /* unit_h */
