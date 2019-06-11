
#include "ai.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>

#define DEBUG_AI 1


/*
 
 
 OLD AI Notes 2
 ------------------------
 
 ACTIONS (* requirement, + increased prioirity)
 

 
 Act and then move
 Or Move and then act
 
 Example Moves:
 Move to enemy -> Attack -> Teleport
 Spawn Enemies -> Move Away/Retreat
 
 typedef struct
 {
 // 0 means impossible, otherwise rank how important this task is
 int (*score)()
 
 int (*execute)()
 } AiAction;
 
 Minion
 - explore
 - attack
 - retreat
 
 
 OLD AI Notes
 ------------------------
 
 STRATEGY
 - High level battlefield reports
 - What info would a general use? (stats, etc)
 
 1. What is my game objective? (win condition)
 2. Does my currenet plan assist in the objective?
 Y) continue plan  N) select new plan
 
 TACTICS
 - unit conditions

 3. Is any unit in an exceptional situation?
 Y) do that
 4. What is the best move for the current plan?
 */

typedef enum
{
    kAiCondNone = 0,
    kAiCondLowHp = 1 << 0,
    kAiCondMedHp = 1 << 1,
    kAiCondHighHp = 1 << 2,
    kAiCondHasAction = 1 << 3,
    kAiCondCanMove = 1 << 4,
    kAiCondCanMoveFar = 1 << 5,
    kAiCondTargetInRange = 1 << 6,
    kAiCondTargetInCloseRange = 1 << 7,
    kAiCondTargetClose = 1 << 8,
    kAiCondObstacleBetweenTarget = 1 << 9,
    kAiCondTargetLowHp = 1 << 10,
    kAiCondTargetMedHp = 1 << 11,
    kAiCondTargetHighHp = 1 << 12,
    kAiCondInsideHealer = 1 << 13,
    kAiCondHealerInMoveRange = 1 << 14,
    kAiCondWeaponRanged = 1 << 15,
    kAiCondSkullNear = 1 << 16,
    kAiCondTargetInMoveRange = 1 << 17,
} AiCond;

typedef struct
{
    const Unit* target;
    const Unit* closest;
    
    int enemiesVisible;
    int alliesVisible;
    
    int alliesClose;
    int enemiesClose;
    
    int alliesTotal;
    
    AiCond conds;
} AiReport;

typedef enum
{
    kAiActionNone = -1,
    kAiActionCharge = 0,
    kAiActionAttack,
    kAiActionRetreat,
    kAiActionIdle,
    kAiActionEvade,
    kAiActionHeal,
    kAiActionTeleport,
    kAiActionSpawn,
    kAiActionHunt
    
} AiActionType;

typedef struct
{
    int (*score)(const Player* controller, const Unit* unit, const AiReport* report);
    Command (*command)(const Player* controller, const Unit* unit, const AiReport* report);
} AiAction;

static AiReport Ai_BuildReport(const Player* controller, const Unit* unit)
{
    Engine* engine = controller->engine;
    
    AiReport report;
    report.target = NULL;
    report.closest = NULL;
    report.enemiesVisible = 0;
    report.alliesVisible = 0;
    
    report.alliesClose = 0;
    report.enemiesClose = 0;
    
    report.conds = 0;
    
    if (unit->hp < unit->maxHp / 3)
    {
        report.conds |= kAiCondLowHp;
    }
    else if (unit->hp < (unit->maxHp * 2) / 3)
    {
        report.conds |= kAiCondMedHp;
    }
    else
    {
        report.conds |= kAiCondHighHp;
    }
    
    if (unit->moveCounter > V_EPSILON)
        report.conds |= kAiCondCanMove;
    
    if (unit->actionCounter > 0)
        report.conds |= kAiCondHasAction;
    
    if (unit->moveCounter > unit->moveRange / 2.0f)
        report.conds |= kAiCondCanMoveFar;
    
    const WeaponInfo* primaryWeapon = controller->engine->weaponTable + unit->primaryWeapon;

    if (primaryWeapon->style == kWeaponStyleRanged)
        report.conds |= kAiCondWeaponRanged;
    
    float targetDist = HUGE_VALF;
    float closestDist = HUGE_VALF;
    

    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        Unit* other = engine->sceneSystem.units + i;
        if (other->dead || other == unit) continue;
        if (other->state == kUnitStateDead) continue;
        
        float distSq = Vec3_DistSq(unit->position, other->position);
        int inRange = distSq < unit->viewRadius * unit->viewRadius;
        
        float closeMoveRange = unit->moveRange + 0.5f;
        int closeRange = distSq < closeMoveRange * closeMoveRange;

        int canSee = unit->isAlerted ? inRange : Unit_CanSee(unit, other, 1.0f);
        
        if (other->playerId != controller->playerId)
        {
            if (canSee && (!report.target || distSq < targetDist))
            {
                report.target = other;
                targetDist = distSq;
            }
            
            if (!report.closest || distSq < closestDist)
            {
                report.closest = other;
                closestDist = distSq;
            }
            
            if (canSee)
                ++report.enemiesVisible;
            
            if (closeRange && canSee)
                ++report.enemiesClose;
        }
        else
        {
            ++report.alliesTotal;

            if (inRange)
                ++report.alliesVisible;;
            
            if (closeRange)
                ++report.alliesClose;
            
        }
    }
    
    if (report.target)
    {
        float targetDistSq = Vec3_DistSq(unit->position, report.target->position);
        
        // if we are out of range, don't attack
        if (targetDistSq < primaryWeapon->range * primaryWeapon->range)
            report.conds |= kAiCondTargetInRange;
        
        if (targetDist < unit->moveRange * unit->moveRange)
            report.conds |= kAiCondTargetInMoveRange;
        
        if (report.conds & kAiCondTargetInRange)
        {
            if (primaryWeapon->style == kWeaponStyleMelee)
            {
                report.conds |= kAiCondTargetInCloseRange;
            }
            else
            {
                float closeRange = (primaryWeapon->range * 0.4f);
                if (targetDistSq < closeRange * closeRange)
                    report.conds |= kAiCondTargetInCloseRange;
            }
        }

        float closeDist = unit->moveRange * 0.4f;
        if (targetDistSq < closeDist * closeDist)
            report.conds |= kAiCondTargetClose;
        
        if (NavSystem_LineIntersectsSolid(&controller->engine->navSystem, AABB_Center(unit->bounds), AABB_Center(report.target->bounds), 5.0f))
            report.conds |= kAiCondObstacleBetweenTarget;
        
        if (report.target->hp < report.target->maxHp / 3)
        {
            report.conds |= kAiCondTargetLowHp;
        }
        else if (report.target->hp < (report.target->maxHp * 2) / 3)
        {
            report.conds |= kAiCondTargetMedHp;
        }
        else
        {
            report.conds |= kAiCondTargetHighHp;
        }
    }
    
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
    {
        const Prop* prop = controller->engine->sceneSystem.props + i;
        if (prop->dead) continue;
        
        if (prop->type == kPropEggHealer)
        {
            float distSq = Vec3_DistSq(unit->position, prop->position);
            float healerRange = 18.0f;
            
            if (distSq < healerRange * healerRange)
                report.conds |= kAiCondInsideHealer;
            
            if (distSq < unit->moveRange * unit->moveRange)
                report.conds |= kAiCondHealerInMoveRange;
        }
        
        if (prop->type == kPropSkull)
        {
            float distSq = Vec3_DistSq(unit->position, prop->position);

            if (distSq < unit->moveRange * unit->moveRange)
                report.conds |= kAiCondSkullNear;
        }
    }
    

    return report;
}

static Unit* Ai_FindNextUnit(const Player* controller)
{
    Engine* engine = controller->engine;

    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        Unit* other = engine->sceneSystem.units + i;
        
        if (other->dead || other->playerId != controller->playerId || other->state == kUnitStateDead)
            continue;

        if (other->doneForTurn == 0)
            return other;
    }
    
    return NULL;
}

/*
 
 Melee Attack: use our claw/fist to attack
 - if we can act
 - if we see an enemy
 - if we are in range
 
 Max: 18
 Min: 10
 
 */
static int Ai_ScoreAttack(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (!(report->conds & kAiCondHasAction) || report->target == NULL)
        return 0;

    if (!(report->conds & kAiCondTargetInRange))
        return 0;

    if (report->conds & kAiCondObstacleBetweenTarget)
        return 0;
    
    int score = 7;
    
    if (report->conds & kAiCondTargetInCloseRange)
        score += 3;
    
    if (report->conds & kAiCondTargetLowHp)
        score += 1;
    
    if (DEBUG_AI)
        printf("attack: %i\n", score);
    
    return score;
}

static Command Ai_Attack(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (DEBUG_AI)
        printf("%s: attack\n", unit->name);
    
    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.type = kCommandTypeAttackPrimary;
    newCommand.position = report->target->position;
    newCommand.target = report->target;
    newCommand.aiAction = kAiActionAttack;
    return newCommand;
}

/* 
     Charge: move towards the nearest enemy
     - if we can see an enemy
     - if we can move
     - bonus if we have more health
 */

static int Ai_ScoreCharge(const Player* controller, const Unit* unit, const AiReport* report)
{
    // we cannot perform this action if we cannot move, and should not if we can't attack
    if (!(report->conds & kAiCondCanMove) || report->target == NULL)
        return 0;
    
    if (report->conds & kAiCondTargetInCloseRange)
        return 0;
    
    int score = 0;
    
    if (!(report->conds & kAiCondTargetInRange))
        score += 3;
    
    if (report->conds & kAiCondHasAction)
    {
        score += 2;
        
        if (report->conds & kAiCondTargetMedHp)
            score += 1;
        
        if (report->conds & kAiCondTargetLowHp)
            score += 2;
    }
    
    if (report->conds & kAiCondHighHp)
        score += 1;
    
    if (report->alliesClose > 2)
        score += 1;
    
    if (DEBUG_AI)
        printf("charge: %i\n", score);
    
    return score;
}

static Command Ai_Charge(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (DEBUG_AI)
        printf("%s: charge\n", unit->name);
    
    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.type = kCommandTypeMove;
    newCommand.target = report->target;
    newCommand.aiAction = kAiActionCharge;
    
    // backup if something fails
    newCommand.position = report->target->position;

    if (report->conds & kAiCondWeaponRanged && report->target->navPoly)
    {
        NavSystem* nav = &controller->engine->navSystem;
        
        NavPath tempPath;
        
        // find a path from the target to this unit
        int status = NavSystem_FindPath(nav,
                                        1.0f,
                                       report->target->position,
                                       unit->position,
                                       report->target->navPoly,
                                       unit->navPoly,
                                       &tempPath);
        
        if (status)
        {
            const WeaponInfo* primaryWeapon = controller->engine->weaponTable + unit->primaryWeapon;
            
            Vec3 point = NavPath_PointAt(&tempPath, primaryWeapon->range * 0.7f);
            newCommand.position = point;
        }
    }

    return newCommand;
}


/*
    Approach: get in range of our weapon
    - if we can act
    - if we can move
*/

/*
 Evade: move away from the enemy, but stay in range
 - if we can move
 - if the enemy is getting too close
 
 TODO: this one often gets them bunched up
*/

static int Ai_ScoreEvade(const Player* controller, const Unit* unit, const AiReport* report)
{
    // we cannot perform this action if we cannot move, and should not if we can't attack
    if (!(report->conds & kAiCondCanMove) || report->target == NULL)
        return 0;
    
    int score = 1;

    if (!(report->conds & kAiCondTargetInCloseRange))
        score += 4;
    
    if (report->conds & kAiCondTargetClose)
        score += 4;

    if (DEBUG_AI)
        printf("evade: %i\n", score);
    
    return score;
}

static Command Ai_Evade(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (DEBUG_AI)
        printf("%s: evade\n", unit->name);
    
    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.type = kCommandTypeMove;
    newCommand.aiAction = kAiActionEvade;
    newCommand.target = NULL;
    
    /*
     this algorithim tries to find a point on the nav mesh, on a circle around the target
     It starts with the angle from the target to the unit, and then moves around the circle
     looking for open points.
     */
    
    Vec3 from = report->target->position;
    Vec3 to = unit->position;
    
    float r;
    
    if (report->conds & kAiCondWeaponRanged)
    {
        const WeaponInfo* primaryWeapon = controller->engine->weaponTable + unit->primaryWeapon;
        r = 0.85f * primaryWeapon->range;
    }
    else
    {
        r = 0.65f * unit->moveRange;
    }
    
    float testZ = MAX(unit->position.z, report->target->position.z) + 1.0f;
    
    float angle = atan2f(to.y - from.y, to.x - from.x);
    
    float diff = 0.0f;
    
    const int binaryDistanceSteps = 4;
    const int angleSteps = 12;
    
    for (int i = 0; i < binaryDistanceSteps; ++i)
    {
        for (int j = 0; j < angleSteps; ++j)
        {
            diff += M_PI / (float)angleSteps;
            
            // move in both the + and - direction around the circle
            Vec2 delta[2];
            delta[0] = Vec2_Create(r * cosf(angle + diff), r * sinf(angle + diff));
            delta[1] = Vec2_Create(r * cosf(angle - diff), r * sinf(angle - diff));
            
            for (int k = 0; k < 2; ++k)
            {
                Ray3 rayP;
                rayP.origin = Vec3_Create(from.x + delta[k].x, from.y + delta[k].y, testZ);
                rayP.dir = Vec3_Create(0.0f, 0.0f, -1.0f);
                
                NavRaycastResult result;
                if (NavSystem_Raycast(&controller->engine->navSystem, rayP, &result))
                {
                    newCommand.position = result.point;
                    goto foundPoint;
                }
            }
        }
        
        r /= 2.0f;
    }
    
    newCommand.position = from;
    newCommand.target = report->target;
    
foundPoint:
    
    return newCommand;
}

/*
    Circle: try to walk behind the enemy, staying out of range
    - if we can move
    - if the enemy is in a group
*/

static int Ai_ScoreCircleEvade(const Player* controller, const Unit* unit, const AiReport* report)
{
    // we cannot perform this action if we cannot move, and should not if we can't attack
    if (!(report->conds & kAiCondCanMove) || report->target == NULL)
        return 0;
    
    if (report->conds & kAiCondHasAction && report->conds & kAiCondTargetInCloseRange)
        return 0;
    
    int score = 1;
    
    if (report->enemiesVisible > 2)
    {
        score += 4;
    }
    else if (!(report->conds & kAiCondHasAction))
    {
        score += 3;
    }
    
    if (report->conds & kAiCondTargetHighHp)
        score += 2;
    
    if (report->conds & kAiCondTargetMedHp)
        score += 1;
    
    if (!(report->conds & kAiCondTargetInMoveRange) || report->conds & kAiCondTargetInCloseRange)
        score += 3;
    
    if (DEBUG_AI)
        printf("circle: %i\n", score);
    
    return score;
}

/*
 
    Retreat: run away from enemies, as far as possible
    - if we can move
    - if there are too many enemies
    - if we don't have very many friends
 
 */

static int Ai_ScoreRetreat(const Player* controller, const Unit* unit, const AiReport* report)
{
    // we cannot perform this action if we cannot move, and should not if we can't attack
    if (!(report->conds & kAiCondCanMove) || report->target == NULL)
        return 0;
    
    if (unit->navPoly == NULL || unit->navPoly->edgeCount == 0)
        return 0;
    
    int score = 0;
    
    if (report->conds & kAiCondLowHp)
        score += 2;
    
    if (report->conds & kAiCondMedHp)
        score += 1;
    
    if (report->alliesVisible < 1)
        score += 2;
    
    int previousIsRetreat = (controller->previousCommand.unit == unit && controller->previousCommand.aiAction == kAiActionRetreat);
    
    if (report->conds & kAiCondTargetClose || previousIsRetreat)
        score += 1;

    if ((report->enemiesVisible > 1 && report->enemiesVisible > 3 * report->alliesVisible + 1) || previousIsRetreat)
        score += 3;
   
    if (!(report->conds & kAiCondCanMoveFar))
        score -= 1;

    if (DEBUG_AI)
        printf("retreat: %i\n", score);
    
    return score;
}

static Command Ai_Retreat(const Player* controller, const Unit* unit, const AiReport* report)
{
    const NavSystem* nav = &controller->engine->navSystem;
    
    const NavPoly* currentPoly = unit->navPoly;
    const NavPoly* destPoly = NULL;
    
    float destPolyDistSq = 0.0f;
    
    for (int i = 0; i < currentPoly->edgeCount; ++i)
    {
        int neighborIndex = nav->navMesh.edges[currentPoly->edgeStart + i].neighborIndex;
        
        if (neighborIndex == -1)
            continue;
        
        const NavPoly* potentialPoly = nav->navMesh.polys + neighborIndex;
        
        // find the neighboring poly that is furthest away from enemies.
        float closestEnemyDistSq = HUGE_VALF;
        
        for (int j = 0; j < SCENE_SYSTEM_UNITS_MAX; ++j)
        {
            const Unit* other = controller->engine->sceneSystem.units + j;
            if (other->dead || other->playerId == controller->playerId) continue;

            float enemyDistSq = Vec3_DistSq(potentialPoly->plane.point, other->position);
            if (enemyDistSq < closestEnemyDistSq)
                closestEnemyDistSq = enemyDistSq;
        }
        
        if (closestEnemyDistSq > destPolyDistSq)
        {
            destPoly = potentialPoly;
            destPolyDistSq = closestEnemyDistSq;
        }
    }
    
    if (DEBUG_AI)
        printf("%s: retreat\n", unit->name);
    
    
    // pull result to edge for some variation
    const NavEdge* edge = nav->navMesh.edges + destPoly->edgeStart + (rand() % destPoly->edgeCount);
    Vec3 edgePoint = nav->navMesh.vertices[edge->vertices[rand() % 2]];
    
    float dist = rand() / (float)RAND_MAX;
    Vec3 finalPoint = Vec3_Lerp(destPoly->plane.point, edgePoint, dist * .8f);
    
    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.target = NULL;
    newCommand.type = kCommandTypeMove;
    newCommand.position = finalPoint;
    newCommand.aiAction = kAiActionRetreat;
    return newCommand;
}


/*
 Protect: If there is a skull nearby, stay near it
 
 */


/*
Heal: move into range of a healer
- if we can move
- if a healer is in range
- if we have less than full hp
 */

static int Ai_ScoreHeal(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (!(report->conds & kAiCondCanMove))
        return 0;
    
    if (!(report->conds & kAiCondHealerInMoveRange))
        return 0;
    
    if (report->conds & kAiCondInsideHealer)
        return 0;
    
    int score = 0;
    
    if (report->conds & kAiCondLowHp)
    {
        score += 7;
    }
    else if (report->conds & kAiCondMedHp)
    {
        score += 4;
    }
    else
    {
        score += 3;
    }
    
    if (report->conds & kAiCondHasAction)
        score -= 3;
    
    if (DEBUG_AI)
        printf("heal: %i\n", score);
    
    return score;
}

static Command Ai_Heal(const Player* controller, const Unit* unit, const AiReport* rpoert)
{
    const Prop* closestTarget = NULL;
    float bestDist = 0;
    
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
    {
        const Prop* prop = controller->engine->sceneSystem.props + i;
        
        if (prop->dead) continue;
        if (prop->type == kPropEggHealer)
        {
            float dist = Vec3_DistSq(unit->position, prop->position);
            
            if (dist < bestDist || closestTarget == NULL)
            {
                closestTarget = prop;
                bestDist = dist;
            }
        }
    }
    
    float closeRange = 12.0f;
    float healerRange = 18.0f;
    
    if (bestDist > healerRange * healerRange)
    {
        NavSystem* nav = &controller->engine->navSystem;
                
        NavPath tempPath;
        if (NavSystem_FindPath(nav,
                               1.0f,
                               closestTarget->position,
                               unit->position,
                               closestTarget->navPoly,
                               unit->navPoly,
                               &tempPath))
        {
            
        }
        
        Vec3 point = NavPath_PointAt(&tempPath, closeRange);
        
        Command newCommand;
        newCommand.playerId = controller->playerId;
        newCommand.unit = unit;
        newCommand.target = NULL;
        newCommand.type = kCommandTypeMove;
        newCommand.position = point;
        return newCommand;
    }
    
    assert(0); // TODO
}

/*
 
 Teleport
 * If we can act
 + surrounded by enemies
 + taken a lot of damage this turn
 + haven't teleported too recently
 */

static int Ai_ScoreTeleport(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (!(report->conds & kAiCondHasAction))
        return 0;
    
    int score = 0;
    
    if (report->target)
        score += 1;

    if (abs(unit->previousTeleportHp - unit->hp) > unit->maxHp / 4)
    {
        score += 8;
    }
    else if (abs(unit->previousTeleportHp - unit->hp) > unit->maxHp / 2)
    {
        score += 9;
    }
    
    printf("teleport: %i\n", score);

    return score;
}

static Command Ai_Teleport(const Player* controller, const Unit* unit, const AiReport* report)
{
    const Engine* engine = controller->engine;
    
    const NavPoly* teleportPoly = NULL;
    const NavPoly* current = unit->navPoly;
    
    short trials = 0;
    
    do
    {
        trials++;
        int index = rand() % engine->navSystem.navMesh.polyCount;
        
        // ignore the current polygon
        if (index == current->index) continue;

        teleportPoly = engine->navSystem.navMesh.polys + index;
        
        // if we have searched and not found un unblocked one, give up, select this one
        if (trials > engine->navSystem.navMesh.polyCount) break;

        // ensure the polygon is not a direct neighbore
        for (int i = 0; i < current->edgeCount; ++i)
        {
            if (engine->navSystem.navMesh.edges[current->edgeStart + i].neighborIndex == index)
            {
                teleportPoly = NULL;
                break;
            }
        }
        
        if (teleportPoly == NULL) continue;
        
        // Make sure we will not be blocked in moving there
        Vec3 center = teleportPoly->plane.point;
        AABB bounds = AABB_CreateCentered(center, Vec3_Create(3.0f, 3.0f, 3.0f));
        
        for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
        {
            const Unit* other = engine->sceneSystem.units + i;
            
            if (!other->dead && other->navPoly == teleportPoly)
            {
                if (AABB_IntersectsAABB(bounds, other->bounds))
                {
                    teleportPoly = NULL;
                    break;
                }
            }
        }
        
    } while (teleportPoly == NULL);
    
    if (DEBUG_AI)
        printf("%s: teleport\n", unit->name);

    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.target = NULL;
    newCommand.type = kCommandTypeTeleport;
    newCommand.position = teleportPoly->plane.point;
    newCommand.aiAction = kAiActionTeleport;
    return newCommand;
}

/*
 Spawn Monsters
 * If we can act
 * If there are not enough monsters
 * If we are out of combat (alone) or If we are alone in combat
 + If we haven't spawned too recently
*/

static int Ai_ScoreSpawnAlly(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (!(report->conds & kAiCondHasAction))
        return 0;
    
    if (report->alliesTotal > 7)
        return 0;
    
    int score = 0;
    
    if (report->enemiesVisible > 0)
    {
        score += 4;

        if (report->alliesTotal < 2)
            score += 2;
        
        if (report->alliesClose < 2)
            score += 1;
    }
    if (DEBUG_AI)
        printf("spawn: %i\n", score);
    
    return score;
}

static Command Ai_SpawnAlly(const Player* controller, const Unit* unit, const AiReport* report)
{
    const Engine* engine = controller->engine;
    
    int numSlots = 20;
    
    int found = 0;
    Vec3 openPoint = Vec3_Zero;
    
    int trials = 0;
    do
    {
        ++trials;
        float radius = 10.0f;
        float angle = (rand() % numSlots) / 10.0f * M_PI;
        
        Vec3 queryPoint = Vec3_Add(unit->position, Vec3_Create(radius * cosf(angle), radius * sinf(angle), 1.0f));
        Ray3 ray =  Ray3_Create(queryPoint, Vec3_Create(0.0f, 0.0f, -1.0f));
        
        NavRaycastResult hitInfo;
        
        if (NavSystem_Raycast(&engine->navSystem, ray, &hitInfo))
        {
            found = 1;
            openPoint = hitInfo.point;
            
            if (trials < numSlots)
            {
                AABB bounds = AABB_CreateCentered(openPoint, Vec3_Create(2.0f, 2.0f, 2.0f));
                
                for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
                {
                    const Unit* other = engine->sceneSystem.units + i;
                    
                    if (!other->dead && AABB_IntersectsAABB(other->bounds, bounds))
                    {
                        found = 0;
                        break;
                    }
                }
            }
        }
        
    } while (!found);
    
    if (DEBUG_AI)
        printf("%s: spawn\n", unit->name);
    
    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.type = kCommandTypeAttackSecondary;
    newCommand.position = openPoint;
    newCommand.aiAction = kAiActionSpawn;
    return newCommand;
}

/*
 
 Hunt: walk around the map, looking for enemies
 - if we can move
 - if no enemies are visible
 */

static int Ai_ScoreHunt(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (!(report->conds & kAiCondCanMove) || !report->closest)
        return 0;
    
    if (!unit->isAlerted)
        return 0;
    
    int score = 0;
    
    if (report->enemiesVisible == 0)
        score += 6;
    
    return score;
}

static Command Ai_Hunt(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (DEBUG_AI)
        printf("%s: hunt\n", unit->name);
    
    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.type = kCommandTypeMove;
    newCommand.target = report->closest;
    newCommand.position = report->closest->position;
    newCommand.aiAction = kAiActionHunt;
    return newCommand;
}

static int Ai_ScoreIdle(const Player* controller, const Unit* unit, const AiReport* report)
{
    if (report->target && unit->actionCounter > 0)
        return 0;
    
    int score = 1;
    
    if (report->conds & kAiCondTargetInCloseRange)
        score += 1;
    
    if (report->conds & kAiCondHighHp)
        score += 1;
    
    if (report->conds & kAiCondInsideHealer)
        score += 3;

    return score;
}

static Command Ai_Idle(const Player* controller, const Unit* unit, const AiReport* ctx)
{
    Command newCommand;
    newCommand.playerId = controller->playerId;
    newCommand.unit = unit;
    newCommand.target = NULL;
    newCommand.type = kCommandTypeIdle;
    newCommand.aiAction = kAiActionIdle;
    return newCommand;
}

static AiAction Ai_batActions[] = {
    {Ai_ScoreAttack, Ai_Attack},
    {Ai_ScoreCharge, Ai_Charge},
    {Ai_ScoreRetreat, Ai_Retreat},
    {Ai_ScoreIdle, Ai_Idle},
    {Ai_ScoreHeal, Ai_Heal},
    {NULL, NULL},
};

static AiAction Ai_vampActions[] = {
    {Ai_ScoreAttack, Ai_Attack},
    {Ai_ScoreEvade, Ai_Evade},
    {Ai_ScoreRetreat, Ai_Retreat},
    {Ai_ScoreIdle, Ai_Idle},
    {Ai_ScoreHeal, Ai_Heal},
    {NULL, NULL},
};

static AiAction Ai_phantomActions[] = {
    {Ai_ScoreAttack, Ai_Attack},
    {Ai_ScoreCharge, Ai_Charge},
    {Ai_ScoreIdle, Ai_Idle},
    {Ai_ScoreHeal, Ai_Heal},
    {NULL, NULL},
};

static AiAction Ai_wolfActions[] = {
    {Ai_ScoreAttack, Ai_Attack},
    {Ai_ScoreCharge, Ai_Charge},
    {Ai_ScoreCircleEvade, Ai_Evade},
    {Ai_ScoreIdle, Ai_Idle},
    {Ai_ScoreHeal, Ai_Heal},
    {NULL, NULL},
};

static AiAction Ai_bossActions[] = {
    {Ai_ScoreAttack, Ai_Attack},
    {Ai_ScoreCharge, Ai_Charge},
    {Ai_ScoreSpawnAlly, Ai_SpawnAlly},
    {Ai_ScoreTeleport, Ai_Teleport},
    {Ai_ScoreIdle, Ai_Idle},
    {Ai_ScoreHunt, Ai_Hunt},
    {NULL, NULL},
};

static int Ai_NextMove(Player* controller, Unit* unit)
{
    if (!unit)
        return 0;
    
    Engine* engine = controller->engine;
    AiReport report = Ai_BuildReport(controller, unit);
    
    const AiAction* action = NULL;
    
    switch (unit->type)
    {
        case kUnitBat:
            action = Ai_batActions;
            break;
        case kUnitVamp:
            action = Ai_vampActions;
            break;
        case kUnitPhantom:
            action = Ai_phantomActions;
            break;
        case kUnitWolf:
            action = Ai_wolfActions;
            break;
        case kUnitBoss:
            action = Ai_bossActions;
            break;
        default:
            action = Ai_batActions;
            break;
    }
    
    int bestScore = 0;
    const AiAction* bestAction = NULL;
    
    while (action->score != NULL)
    {
        int score = action->score(controller, unit, &report);
        
        // save the best score
        // resolve ties with a dice roll
        if (score > 0 && (score > bestScore || (score == bestScore && rand() % 2 == 1)))
        {
            bestScore = score;
            bestAction = action;
        }
        
        ++action;
    }

    if (bestAction == NULL)
    {
        unit->doneForTurn = 1;
        return 0;
    }
    
    Command toRun = bestAction->command(controller, unit, &report);
    
    if (toRun.type == kCommandTypeIdle)
    {
        unit->doneForTurn = 1;
        return 0;
    }
    else
    {
        if (DEBUG_AI)
            printf("\n");
        
        Engine_RunCommand(engine, &toRun);
        return 1;
    }
}

static void Ai_OnEvent(Player* controller, const PlayerEvent* event)
{
    Engine* engine = controller->engine;
    
    switch (event->type)
    {
        case kPlayerEventJoin:
        {
            break;
        }
        case kPlayerEventStartTurn:
        {
            for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
            {
                Unit* other = engine->sceneSystem.units + i;
                if (other->dead || other->playerId != controller->playerId) continue;
                other->doneForTurn = 0;
            }
            
            controller->selection = SceneSystem_FindUnitType(&engine->sceneSystem, kUnitBoss, controller->playerId);
            
            if (!controller->selection)
                controller->selection = SceneSystem_FindUnitType(&engine->sceneSystem, -1, controller->playerId);
            
            Ai_NextMove(controller, controller->selection);

            break;
        }
        case kPlayerEventEndTurn:
        {
            if (DEBUG_AI)
                printf("Ai done\n\n");            
            break;
        }
        case kPlayerEventStartCommand:
        {
            break;
        }
        case kPlayerEventEndCommand:
        {
            int foundCommand = 0;
            while (!foundCommand)
            {
                if (Ai_NextMove(controller, controller->selection))
                {
                    foundCommand = 1;
                }
                else
                {
                    controller->selection = Ai_FindNextUnit(controller);

                    if (!controller->selection)
                    {
                        foundCommand = 1;
                        
                        // no more moves, end turn
                        Command newCommand;
                        newCommand.type = kCommandTypeEndTurn;
                        newCommand.playerId = controller->playerId;
                        Engine_RunCommand(engine, &newCommand);
                    }
                }
            }
            break;
        }
        case kPlayerEventTap:
        {
            break;
        }
    }
}

void Ai_Init(Player* controller)
{
    Player_Init(controller);
    controller->onEvent = Ai_OnEvent;
}
