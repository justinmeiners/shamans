
#include "unit.h"
#include "engine.h"


void Unit_Init(Unit* unit, struct Engine* engine, int index)
{
    unit->playerId = -1;
    unit->crewIndex = -1;
    unit->engine = engine;
        
    unit->target = NULL;
    
    unit->hp = 100;
    
    unit->state = kUnitStateIdle;
    unit->powerups = kUnitPowerupNone;
    
    unit->moveRange = 0;
    
    unit->onSpawn = NULL;
    unit->onKill = NULL;
    unit->onTick = NULL;
    unit->onDamage = NULL;
    
    unit->onStartTurn = NULL;
    unit->onSelect = NULL;
    unit->onDeslect = NULL;
        
    unit->selected = 0;
    unit->radius = 2.0f;
    
    unit->viewRadius = 30;
    unit->viewAngle = 80;
    unit->isAlerted = 0;
    
    unit->primaryWeapon = -1;
    unit->secondaryWeapon = -1;
    
    unit->weaponProp = NULL;
    unit->actionProp = NULL;
    
    unit->angle = 0.0;
    unit->targetAngle = 0.0;
    unit->navPoly = NULL;
    
    unit->dead = 1;
    
    unit->user1 = -1;
    unit->user2 = -1;
    
    unit->index = index;
    
    
    NavPath_Init(&unit->path);
}

void Unit_Kill(Unit* unit)
{
    unit->dead = 1;
    
    if (unit->onKill)
        unit->onKill(unit);
    
    if (unit->weaponProp)
        Prop_Kill(unit->weaponProp);
}

int Unit_StartPath(Unit* unit, Vec3 destination)
{
    Ray3 targetRay = Ray3_Create(Vec3_Add(destination, Vec3_Create(0.0f, 0.0f, 1.0f)), Vec3_Create(0.0f, 0.0f, -1.0f));
        
    const NavPoly* start = unit->navPoly;
    
    if (!start) return 0;

    NavRaycastResult hitInfo;
    if (!NavSystem_Raycast(&unit->engine->navSystem, targetRay, &hitInfo))
        return 0;
    
    NavSystem_FindPath(&unit->engine->navSystem,
                       2.0f,
                       unit->position,
                       destination,
                       unit->navPoly,
                       hitInfo.poly,
                       &unit->path);
    
    unit->state = kUnitStateMove;
    unit->pathIndex = 1;
    
    unit->onStartPath(unit);
    
    return 1;
}

void Unit_Damage(Unit* unit,
                 Prop* other,
                 int damage,
                 DamageType damageType)
{
    if (unit->onDamage)
        unit->onDamage(unit, other, damage, damageType);
}

int Unit_FollowPath(Unit* unit)
{
    if (unit->moveCounter <= 0.0f)
    {
        return 0;
    }
    
    if (unit->target)
    {
        float minDist = unit->radius + unit->target->radius;
        
        if (Vec3_DistSq(unit->position, unit->target->position) <= minDist * minDist)
            return 0;
    }
    
    // move along path
    Vec3 dest = unit->path.nodes[unit->pathIndex].point;
    Vec3 dir = Vec3_Sub(dest, unit->position);
    
    const float closeEnoughDist = 0.15f;
    
    if (Vec2_LengthSq(Vec2_FromVec3(dir)) > closeEnoughDist * closeEnoughDist)
    {
        unit->targetAngle = RAD_TO_DEG(atan2(dir.y, dir.x));
        
        if (unit->pathIndex == 0)
        {
            unit->angle = unit->targetAngle;
        }
        else
        {
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            unit->angle = Deg_Normalize(unit->angle + CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED));
        }
        
        dir = Vec3_Norm(dir);
        float z = unit->position.z;
        unit->position = Vec3_Add(unit->position, Vec3_Scale(dir, unit->speed));
        unit->position.z = z;
        unit->moveCounter -= unit->speed;
    }
    else
    {
        unit->position = dest;
        
        if (unit->pathIndex + 1 < unit->path.nodeCount)
        {
            unit->pathIndex++;
        }
        else
        {
            return 0;
        }
    }

    return 1;
}

void Unit_CancelMove(Unit* unit)
{
    unit->target = NULL;
    unit->pathIndex = 0;
    unit->state = kUnitStateIdle;
}

void Unit_SetName(Unit* unit, const char* name)
{
    strncpy(unit->name, name, UNIT_NAME_MAX);
}

int Unit_CanSee(const Unit* unit, const Unit* other, float range)
{
    Vec3 direction = Vec3_Sub(other->position, unit->position);
    
    float viewDistSq = range * range;
    
    float distSq = Vec3_LengthSq(direction);
    if (distSq < viewDistSq)
    {
        float angle = acosf(Vec3_Dot(Vec3_Norm(direction), unit->forward));
        
        if (angle < DEG_TO_RAD(unit->viewAngle))
            return 1;
    }

    return 0;
}
