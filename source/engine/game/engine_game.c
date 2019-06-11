
#include "engine.h"
#include <assert.h>


// damage  is more impactful in the back than the front
// range: (1.0 - strength, 1.0)
static inline float CalcDirectionFactor(Vec3 v0, Vec3 v1, float strength)
{
    float forwardDot = (Vec2_Dot(Vec2_FromVec3(v0), Vec2_FromVec3(v1)) + 1.0) / 2.0f;
    return (1.0f - strength) + (forwardDot * strength);
}

static void Scientist_OnKill(Unit* unit)
{
    PartSystem_EmitEffect(&unit->engine->partSystem, kPartEffectSmoke, Vec3_Offset(unit->position, 0.0, 0.0, 1.0), Quat_Identity, 0);
}

static void Scientist_OnTick(Unit* unit)
{
    unit->bounds = AABB_CreateAnchored(unit->position, Vec3_Create(3.0f, 3.0f, 6.5), Vec3_Create(0.5f, 0.5f, 0.0f));
    
    const SkelAttachPoint* point = SkelModel_AttachPointAt(&unit->skelModel, 0);
    unit->weaponProp->position = Vec3_Add(unit->position, point->modelPosition);
    unit->weaponProp->rotation = point->modelRotation;
    
    short walkAnim = ANIM_SCIENTIST_WALK;
    short idleAnim = ANIM_SCIENTIST_IDLE;
    short dieAnim = ANIM_SCIENTIST_DIE;
    short hurtAnim = ANIM_SCIENTIST_HURT;
    
    if (unit->primaryWeapon == kWeaponRevolver)
    {
        walkAnim = ANIM_SCIENTIST_PISTOL_WALK;
        idleAnim = ANIM_SCIENTIST_PISTOL_IDLE;
    }
    
    Engine* engine = unit->engine;
    
    switch (unit->state)
    {
        case kUnitStateIdle:
        {
            // angle correction
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator, engine->renderSystem.anims + walkAnim, 0, 6);
            }
            else
            {
                // randomize frame at the start
                const SkelAnim* idleSkelAnim = engine->renderSystem.anims + idleAnim;
                int startFrame = unit->skelModel.animator.anim ? 0 : SkelAnim_RandomFrame(idleSkelAnim);
                SkelAnimator_SetAnim(&unit->skelModel.animator, idleSkelAnim, startFrame, 12);
                
                if (!SkelAnimator_InTransition(&unit->skelModel.animator))
                {
                    // idle is always in primary weapon
                    
                    int weaponModel = engine->weaponTable[unit->primaryWeapon].model;
                    StaticModel_Shutdown(&unit->weaponProp->model);
                    StaticModel_Copy(&unit->weaponProp->model, engine->renderSystem.models + weaponModel);
                    unit->weaponProp->model.material.diffuseMap = engine->weaponTable[unit->primaryWeapon].texture;
                }
            }
            
            SkelModel_Tick(&unit->skelModel);
            break;
        }
        case kUnitStateDead:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator, engine->renderSystem.anims + dieAnim, 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) Unit_Kill(unit);
            break;
        }
        case kUnitStateHurt:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[hurtAnim], 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) unit->state = kUnitStateIdle;
            break;
        }
        case kUnitStateMove:
        {
            if (!Unit_FollowPath(unit))
                unit->state = kUnitStateIdle;
            
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

            if (frameInfo.marker != NULL)
            {
                if (strcmp(frameInfo.marker->name, "step1") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP1);
                }
                else if (strcmp(frameInfo.marker->name, "step2") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP2);
                }
            }
            break;
        }
        case kUnitStateAttackPrimary:
        case kUnitStateAttackSecondary:
        {
            int weaponIndex = unit->primaryWeapon;
            
            if (unit->state == kUnitStateAttackSecondary)
                weaponIndex = unit->secondaryWeapon;
            
            const WeaponInfo* weapon = engine->weaponTable + weaponIndex;

            int weaponAnim;
            
            switch (weaponIndex)
            {
                case kWeaponRevolver:
                    weaponAnim = ANIM_SCIENTIST_PISTOL_SHOOT;
                    break;
                case kWeaponMg:
                    weaponAnim = ANIM_SCIENTIST_MG_SHOOT;
                    break;
                case kWeaponCannon:
                    weaponAnim = ANIM_SCIENTIST_CANNON_SHOOT;
                    break;
                case kWeaponMachete:
                    weaponAnim = ANIM_SCIENTIST_MACHETE;
                    break;
                case kWeaponAxe:
                    weaponAnim = ANIM_SCIENTIST_AXE;
                    break;
                case kWeaponSyringe:
                    weaponAnim = ANIM_SCIENTIST_SYRINGE;
                    break;
                default:
                    assert(0);
                    break;
            }
            
            Vec3 dir = Vec3_Norm(Vec3_Sub(engine->command.position, unit->position));
            unit->targetAngle = RAD_TO_DEG(atan2(dir.y, dir.x));
            
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            
            /* if we aren't facing the target, we need to rotate */
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                
                SkelAnimator_SetAnim(&unit->skelModel.animator, engine->renderSystem.anims + walkAnim, 0, 6);
                SkelModel_Tick(&unit->skelModel);
            }
            else
            {
                SkelAnimator_SetAnim(&unit->skelModel.animator, engine->renderSystem.anims + weaponAnim, 0, 6);
                
                SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

                if (frameInfo.finishedTransition && unit->weaponProp)
                {
                    StaticModel_Shutdown(&unit->weaponProp->model);
                    StaticModel_Copy(&unit->weaponProp->model, engine->renderSystem.models + weapon->model);
                    unit->weaponProp->model.material.diffuseMap = weapon->texture;
                }
                
                if (frameInfo.finishedAnim)
                {
                    unit->actionCounter--;
                    unit->state = kUnitStateIdle;
                }
                else if (frameInfo.marker != NULL)
                {
                    if (weapon->style == kWeaponStyleRanged)
                    {
                        Vec3 emitPoint = Vec3_Add(unit->weaponProp->position, Quat_MultVec3(&unit->weaponProp->rotation, weapon->projectileOffset));
                        Vec3 endPoint = Vec3_Add(engine->command.position, Vec3_Sub(emitPoint, unit->position));

                        Vec3 direction = Vec3_Norm(Vec3_Sub(endPoint, emitPoint));
                        direction = WeaponInfo_CalcRandSpread(weapon->spread, direction);
                        
                        Quat rotation = Quat_CreateLook(direction, Vec3_Create(1.0f, 0.0f, 0.0f));
                        
                        Prop* projectile = SceneSystem_SpawnPropAt(&engine->sceneSystem, weapon->projectilePropType, emitPoint, rotation, 0);
                        projectile->owner = unit;
                        projectile->hp = weapon->damage;
                        
                        if (rand() % weapon->critChance == 1)
                        {
                            projectile->hp += weapon->critBonus;
                        }
                    }
                    else
                    {
                        Vec3 p = Vec3_Add(unit->weaponProp->position, Quat_MultVec3(&unit->weaponProp->rotation, weapon->projectileOffset));
                        
                        Prop* projectile = SceneSystem_SpawnPropAt(&engine->sceneSystem, weapon->projectilePropType, p, unit->weaponProp->rotation, 0);
                        projectile->owner = unit;
                        projectile->hp = weapon->damage;
                    }
                }
            }
        }
        default: break;
    }
}

static void Scientist_OnSelect(Unit* unit)
{
    int sounds[] = {
        SND_SCIENTIST_SELECT1,
        SND_SCIENTIST_SELECT2,
        SND_SCIENTIST_SELECT3,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }
    
    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}

static void Scientist_OnStartPath(Unit* unit)
{
    int sounds[] = {
        SND_SCIENTIST_MOVE1,
        SND_SCIENTIST_MOVE2,
        SND_SCIENTIST_MOVE3,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }

    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}


static void Scientist_OnDamage(struct Unit* unit,
                         struct Prop* prop,
                         int damage,
                         DamageType damageType)
{
    if (unit->state == kUnitStateDead)
        return;
    
    Engine* engine = unit->engine;
    
    int defensePowerup = unit->powerups & kUnitPowerupDefense;
    
    switch (damageType)
    {
        case kDamageTypeMelee:
        {
            unit->isAlerted = 1;
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.3f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem, (rand() % 2 == 0) ? SND_SCIENTIST_HURT1 : SND_SCIENTIST_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeBullet:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.3f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem, (rand() % 2 == 0) ? SND_SCIENTIST_HURT1 : SND_SCIENTIST_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMagic:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.2f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem, (rand() % 2 == 0) ? SND_SCIENTIST_HURT1 : SND_SCIENTIST_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeAcid:
        {
            unit->isAlerted = 1;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, AABB_Center(unit->bounds), Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem, (rand() % 2 == 0) ? SND_SCIENTIST_HURT1 : SND_SCIENTIST_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMindControl:
        {
            break;
        }
        case kDamageTypeHeal:
        {
            unit->hp += damage;
            
            if (unit->hp > unit->maxHp)
                unit->hp = unit->maxHp;
            break;
        }
        case kDamageTypeOther:
        {
            unit->hp -= damage;
            break;
        }
    }
    
    if (prop->owner)
    {
        Vec2 ownerPoint2D = Vec2_FromVec3(prop->owner->position);
        float angleToOwner = RAD_TO_DEG(atan2f(ownerPoint2D.y - unit->position.y, ownerPoint2D.x - unit->position.x));
        unit->targetAngle = angleToOwner;
    }
    
    if (unit->hp < 1)
    {
        unit->state = kUnitStateDead;
        SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_SCIENTIST_DIE1 : SND_SCIENTIST_DIE2);
    }
}


static void Scientist_OnSpawn(Unit* unit, int flags)
{
    Engine* engine = unit->engine;
    
    engine->renderSystem.cam.target = unit->position;
    
    unit->maxHp = 100;
    unit->hp = unit->maxHp;
    
    unit->onKill = Scientist_OnKill;
    unit->onTick = Scientist_OnTick;
    unit->onSelect = Scientist_OnSelect;
    unit->onStartPath = Scientist_OnStartPath;
    unit->onDamage = Scientist_OnDamage;
    Unit_SetName(unit, "Scientist");
    
    unit->speed = 0.2f;
    unit->moveRange = 40;
    unit->primaryWeapon = kWeaponRevolver;
    unit->secondaryWeapon = kWeaponAxe;
    unit->viewRadius = 50;
    
    SkelModel_Copy(&unit->skelModel, &engine->renderSystem.skelModels[SKEL_SCIENTIST]);
    unit->skelModel.material.diffuseMap = TEX_SCIENTIST;
    
    unit->weaponProp = SceneSystem_SpawnProp(&engine->sceneSystem, kPropWeapon);
    
    int weaponModel = engine->weaponTable[unit->primaryWeapon].model;
    StaticModel_Copy(&unit->weaponProp->model, engine->renderSystem.models + weaponModel);
    unit->weaponProp->model.material.diffuseMap = engine->weaponTable[unit->primaryWeapon].texture;
    
    unit->skelModel.attachPointTable[0] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "attach_r_hand");
}

static void Phantom_OnSelect(Unit* unit)
{
    int sounds[] = {
        SND_PHANTOM_SELECT1,
        SND_PHANTOM_SELECT1,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }
    
    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}

static void Phantom_OnStartPath(Unit* unit)
{
    int sounds[] = {
        SND_PHANTOM_MOVE1,
        SND_PHANTOM_MOVE2,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }
    
    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}

static void Phantom_OnTick(Unit* unit)
{
    unit->bounds = AABB_CreateAnchored(unit->position, Vec3_Create(3.0f, 3.0f, 8.0), Vec3_Create(0.5f, 0.5f, 0.0f));
    
    short walkAnim = ANIM_PHANTOM_WALK;
    short idleAnim = ANIM_PHANTOM_IDLE;
    short attackAnim = ANIM_PHANTOM_ATTACK;
    short dieAnim = ANIM_PHANTOM_DIE;
    short hurtAnim = ANIM_PHANTOM_HURT;
    
    Engine* engine = unit->engine;
    
    switch (unit->state)
    {
        case kUnitStateIdle:
        {
            // angle correction
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
                
            }
            else
            {
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[idleAnim], 0, 12);
                
            }
            
            SkelModel_Tick(&unit->skelModel);
            break;
        }
        case kUnitStateDead:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[dieAnim], 0, 3);
            
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) Unit_Kill(unit);
            break;
        }
        case kUnitStateHurt:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[hurtAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) unit->state = kUnitStateIdle;
            break;
        }
        case kUnitStateMove:
        {
            if (!Unit_FollowPath(unit)) unit->state = kUnitStateIdle;
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            
            if (frameInfo.marker != NULL)
            {
                if (strcmp(frameInfo.marker->name, "step1") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP1);
                }
                else if (strcmp(frameInfo.marker->name, "step2") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP2);
                }
            }
            break;
        }
        case kUnitStateAttackPrimary:
        case kUnitStateAttackSecondary:
        {
            const WeaponInfo* weapon = engine->weaponTable + unit->primaryWeapon;
            
            Vec3 dir = Vec3_Norm(Vec3_Sub(engine->command.position, unit->position));
            unit->targetAngle = RAD_TO_DEG(atan2(dir.y, dir.x));
            
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            
            // if we aren't facing the target, we need to rotate
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
                SkelModel_Tick(&unit->skelModel);
            }
            else
            {
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[attackAnim], 0, 6);
                SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

                if (frameInfo.finishedAnim)
                {
                    unit->actionCounter--;
                    unit->state = kUnitStateIdle;
                }
                else if (frameInfo.marker != NULL)
                {
                    const SkelAttachPoint* attachPoint = SkelModel_AttachPointAt(&unit->skelModel, 0);
                    
                    Vec3 emitPoint = Vec3_Add(attachPoint->modelPosition, unit->position);
                    
                    Prop* projectile = SceneSystem_SpawnPropAt(&engine->sceneSystem, weapon->projectilePropType, emitPoint, attachPoint->modelRotation, 0);
                    projectile->owner = unit;
                    projectile->hp = weapon->damage;
                }
            }
        }
        default: break;
    }
}

static void Phantom_OnKill(Unit* unit)
{
    PartSystem_EmitEffect(&unit->engine->partSystem, kPartEffectSmoke, Vec3_Offset(unit->position, 0.0, 0.0, 1.0), Quat_Identity, 0);
}

static void Phantom_OnDamage(struct Unit* unit,
                             struct Prop* prop,
                             int damage,
                             DamageType damageType)
{
    if (unit->state == kUnitStateDead)
        return;
    
    Engine* engine = unit->engine;
    
    int defensePowerup = unit->powerups & kUnitPowerupDefense;
    
    switch (damageType)
    {
        case kDamageTypeMelee:
        {
            unit->isAlerted = 1;
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.85f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);

            if (directionFactor < 0.5f)
            {
                SndSystem_PlaySound(&unit->engine->soundSystem, SND_HIT_GLANCE);
            }
            else
            {
                SndSystem_PlaySound(&unit->engine->soundSystem, SND_PHANTOM_HURT1);
                unit->state = kUnitStateHurt;
            }
            
            break;
        }
        case kDamageTypeBullet:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.8f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);

            if (directionFactor < 0.5f)
            {
                SndSystem_PlaySound(&unit->engine->soundSystem, SND_HIT_GLANCE);
            }
            else
            {
                SndSystem_PlaySound(&unit->engine->soundSystem, SND_PHANTOM_HURT1);
                unit->state = kUnitStateHurt;
            }
            
            break;
        }
            
        case kDamageTypeMagic:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.5f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);

            if (directionFactor < 0.5f)
            {
                SndSystem_PlaySound(&unit->engine->soundSystem, SND_HIT_GLANCE);
            }
            else
            {
                SndSystem_PlaySound(&unit->engine->soundSystem, SND_PHANTOM_HURT1);
                unit->state = kUnitStateHurt;
            }

            break;
        }
        case kDamageTypeAcid:
        {
            unit->isAlerted = 1;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, AABB_Center(unit->bounds), Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem, SND_PHANTOM_HURT1);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMindControl:
        {
            unit->isAlerted = 1;
            
            int ownersUnitCount = engine->players[prop->owner->playerId]->unitCount;
            int diceRoll = (rand() % 2 == 1);
            
            if (diceRoll && ownersUnitCount < OBSERVERS_MAX)
            {
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectMindControl, unit->position, Quat_Identity, 0);
                SndSystem_PlaySound(&engine->soundSystem, SND_MIND_CONTROL);

                unit->playerId = prop->owner->playerId;
                ++engine->players[unit->playerId]->unitCount;
            }
            else
            {
                unit->state = kUnitStateHurt;
                
                if (defensePowerup)
                    damage /= 2;
                
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
                SndSystem_PlaySound(&unit->engine->soundSystem, SND_PHANTOM_HURT1);
                unit->hp -= damage;
            }

            break;
        }
        case kDamageTypeHeal:
            unit->hp += damage;
            
            if (unit->hp > unit->maxHp)
                unit->hp = unit->maxHp;
            
            break;
        case kDamageTypeOther:
            unit->hp -= damage;
            break;
    }
    
    if (prop && prop->owner)
    {
        Vec2 ownerPoint2D = Vec2_FromVec3(prop->owner->position);
        float angleToOwner = RAD_TO_DEG(atan2f(ownerPoint2D.y - unit->position.y, ownerPoint2D.x - unit->position.x));
        unit->targetAngle = angleToOwner;
    }
    
    if (unit->hp < 1)
    {
        SndSystem_PlaySound(&unit->engine->soundSystem, SND_PHANTOM_DIE1);
        unit->state = kUnitStateDead;
    }
}

static void Phantom_OnSpawn(Unit* unit, int flags)
{
    Engine* engine = unit->engine;
    
    unit->onTick = Phantom_OnTick;
    unit->onSelect = Phantom_OnSelect;
    unit->onStartPath = Phantom_OnStartPath;
    unit->onKill = Phantom_OnKill;
    unit->onDamage = Phantom_OnDamage;
    unit->moveRange = 27;
    unit->maxHp = 160;
    unit->hp = unit->maxHp;
    unit->viewRadius = 50;
    
    Unit_SetName(unit, "Phantom");
    
    unit->primaryWeapon = kWeaponPhantomClaw;
    
    unit->speed = 0.2f;
    
    unit->weaponProp = SceneSystem_SpawnProp(&engine->sceneSystem, kPropWeapon);

    SkelModel_Copy(&unit->skelModel, &engine->renderSystem.skelModels[SKEL_PHANTOM]);
    unit->skelModel.material.diffuseMap = TEX_PHANTOM;
    
    unit->skelModel.attachPointTable[0] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "attach_hand");
}

static void Bat_OnSelect(Unit* unit)
{
    int sounds[] = {
        SND_BAT_SELECT1,
        SND_BAT_SELECT2,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }
    
    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}

static void Bat_OnStartPath(Unit* unit)
{
    int sounds[] = {
        SND_BAT_MOVE1,
        SND_BAT_MOVE2,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }
    
    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}

static void Bat_OnTick(Unit* unit)
{
    unit->bounds = AABB_CreateAnchored(unit->position, Vec3_Create(3.0f, 3.0f, 6.0), Vec3_Create(0.5f, 0.5f, 0.0f));
    
    short walkAnim = ANIM_BAT_WALK;
    short idleAnim = ANIM_BAT_IDLE;
    short attackAnim = ANIM_BAT_ATTACK;
    short dieAnim = ANIM_BAT_DIE;
    short hurtAnim = ANIM_BAT_HURT;
    
    Engine* engine = unit->engine;
    
    switch (unit->state)
    {
        case kUnitStateIdle:
        {
            // angle correction
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            }
            else
            {
                // randomize frame at the start
                const SkelAnim* idleSkelAnim = engine->renderSystem.anims + idleAnim;
                int startFrame = unit->skelModel.animator.anim ? 0 : SkelAnim_RandomFrame(idleSkelAnim);
                SkelAnimator_SetAnim(&unit->skelModel.animator, idleSkelAnim, startFrame, 12);
            }
            
            SkelModel_Tick(&unit->skelModel);
            
            break;
        }
        case kUnitStateDead:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[dieAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) Unit_Kill(unit);
            break;
        }
        case kUnitStateHurt:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[hurtAnim], 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) unit->state = kUnitStateIdle;
            break;
        }
        case kUnitStateMove:
        {
            if (!Unit_FollowPath(unit)) unit->state = kUnitStateIdle;
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

            if (frameInfo.marker != NULL)
            {
                if (strcmp(frameInfo.marker->name, "step1") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP1);
                }
                else if (strcmp(frameInfo.marker->name, "step2") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP2);
                }
            }
            
            break;
        }
        case kUnitStateAttackPrimary:
        case kUnitStateAttackSecondary:
        {
            const WeaponInfo* weapon = engine->weaponTable + unit->primaryWeapon;
            
            Vec3 dir = Vec3_Norm(Vec3_Sub(engine->command.position, unit->position));
            unit->targetAngle = RAD_TO_DEG(atan2(dir.y, dir.x));
            
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            
            // if we aren't facing the target, we need to rotate
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
                SkelModel_Tick(&unit->skelModel);
            }
            else
            {
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[attackAnim], 0, 6);
                SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

                if (frameInfo.finishedAnim)
                {
                    unit->actionCounter--;
                    unit->state = kUnitStateIdle;
                }
                else if (frameInfo.marker != NULL)
                {
                    // Bat uses a two handed attack, the attach point depends on the frame of animation
                    const SkelAttachPoint* attachPoint = SkelModel_AttachPointAt(&unit->skelModel, 1);
                    
                    if (strcmp(frameInfo.marker->name, "attack2"))
                    {
                        attachPoint = SkelModel_AttachPointAt(&unit->skelModel, 0);
                    }
                    
                    Vec3 emitPoint = Vec3_Add(attachPoint->modelPosition, unit->position);
                    
                    Prop* projectile = SceneSystem_SpawnPropAt(&engine->sceneSystem, weapon->projectilePropType, emitPoint, attachPoint->modelRotation, 0);
                    projectile->owner = unit;
                    projectile->hp = weapon->damage;
                }
            }
        }
        default: break;
    }
}

static void Bat_OnKill(Unit* unit)
{
    PartSystem_EmitEffect(&unit->engine->partSystem, kPartEffectSmoke, Vec3_Offset(unit->position, 0.0, 0.0, 1.0), Quat_Identity, 0);
}


static void Bat_OnDamage(struct Unit* unit,
                             struct Prop* prop,
                             int damage,
                             DamageType damageType)
{
    if (unit->state == kUnitStateDead)
        return;
    
    Engine* engine = unit->engine;
    
    int defensePowerup = unit->powerups & kUnitPowerupDefense;
    
    switch (damageType)
    {
        case kDamageTypeMelee:
        {
            unit->isAlerted = 1;
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.4f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeBullet:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.2f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
            
        case kDamageTypeMagic:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.2f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeAcid:
        {
            unit->isAlerted = 1;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, AABB_Center(unit->bounds), Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMindControl:
        {
            unit->isAlerted = 1;
            
            int ownersUnitCount = engine->players[prop->owner->playerId]->unitCount;
            int diceRoll = (rand() % 3 != 1);
            
            if (diceRoll && ownersUnitCount < OBSERVERS_MAX)
            {
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectMindControl, unit->position, Quat_Identity, 0);
                SndSystem_PlaySound(&engine->soundSystem, SND_MIND_CONTROL);
                
                unit->playerId = prop->owner->playerId;
                ++engine->players[unit->playerId]->unitCount;
            }
            else
            {
                unit->state = kUnitStateHurt;
                
                if (defensePowerup)
                    damage /= 2;
                
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
                
                SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
                unit->state = kUnitStateHurt;
                unit->hp -= damage;
            }
            
            break;
        }
        case kDamageTypeHeal:
        {
            unit->hp += damage;
            
            if (unit->hp > unit->maxHp)
                unit->hp = unit->maxHp;
            
            break;
        }
        case kDamageTypeOther:
            unit->hp -= damage;
            break;
    }
    
    if (prop && prop->owner)
    {
        Vec2 ownerPoint2D = Vec2_FromVec3(prop->owner->position);
        float angleToOwner = RAD_TO_DEG(atan2f(ownerPoint2D.y - unit->position.y, ownerPoint2D.x - unit->position.x));
        unit->targetAngle = angleToOwner;
    }
    
    if (unit->hp < 1)
        unit->state = kUnitStateDead;
}

static void Bat_OnSpawn(Unit* unit, int flags)
{
    Engine* engine = unit->engine;
    
    unit->onTick = Bat_OnTick;
    unit->onSelect = Bat_OnSelect;
    unit->onStartPath = Bat_OnStartPath;
    unit->onKill = Bat_OnKill;
    unit->onDamage = Bat_OnDamage;
    unit->moveRange = 41;
    unit->viewRadius = 50;
    unit->maxHp = 35;
    unit->hp = unit->maxHp;
    
    Unit_SetName(unit, "Bat");
    
    unit->primaryWeapon = kWeaponBatClaw;
    
    unit->speed = 0.25f;
    
    unit->weaponProp = SceneSystem_SpawnProp(&engine->sceneSystem, kPropWeapon);

    SkelModel_Copy(&unit->skelModel, &engine->renderSystem.skelModels[SKEL_BAT]);
    unit->skelModel.material.diffuseMap = TEX_BAT;

    unit->skelModel.attachPointTable[0] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "r_hand");
    unit->skelModel.attachPointTable[1] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "l_hand");
}

static void Vamp_OnDamage(struct Unit* unit,
                         struct Prop* prop,
                         int damage,
                         DamageType damageType)
{
    if (unit->state == kUnitStateDead)
        return;
    
    Engine* engine = unit->engine;
    
    int defensePowerup = unit->powerups & kUnitPowerupDefense;
    
    switch (damageType)
    {
        case kDamageTypeMelee:
        {
            unit->isAlerted = 1;
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.1f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeBullet:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.1f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeAcid:
        {
            unit->isAlerted = 1;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, AABB_Center(unit->bounds), Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMagic:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.2f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMindControl:
        {
            unit->isAlerted = 1;
            
            int ownersUnitCount = engine->players[prop->owner->playerId]->unitCount;
            int diceRoll = (rand() % 3 != 1);

            if (diceRoll && ownersUnitCount < OBSERVERS_MAX)
            {
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectMindControl, unit->position, Quat_Identity, 0);
                SndSystem_PlaySound(&engine->soundSystem, SND_MIND_CONTROL);
                
                unit->playerId = prop->owner->playerId;
                ++engine->players[unit->playerId]->unitCount;
            }
            else
            {
                unit->state = kUnitStateHurt;
                
                if (defensePowerup)
                    damage /= 2;
                
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
                
                SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BAT_HURT1 : SND_BAT_HURT2);
                unit->state = kUnitStateHurt;
                unit->hp -= damage;
            }
            
            break;
        }
        case kDamageTypeHeal:
        {
            unit->hp += damage;
            
            if (unit->hp > unit->maxHp)
                unit->hp = unit->maxHp;
            break;
        }
        case kDamageTypeOther:
        {
            unit->hp -= damage;
            break;
        }
    }
    
    if (prop && prop->owner)
    {
        Vec2 ownerPoint2D = Vec2_FromVec3(prop->owner->position);
        float angleToOwner = RAD_TO_DEG(atan2f(ownerPoint2D.y - unit->position.y, ownerPoint2D.x - unit->position.x));
        unit->targetAngle = angleToOwner;
    }
    
    if (unit->hp < 1)
        unit->state = kUnitStateDead;
}


static void Vamp_OnKill(Unit* unit)
{
    PartSystem_EmitEffect(&unit->engine->partSystem, kPartEffectSmoke, Vec3_Offset(unit->position, 0.0, 0.0, 1.0), Quat_Identity, 0);
}

static void Vamp_OnTick(Unit* unit)
{
    unit->bounds.min = Vec3_Add(unit->position, Vec3_Create(-1.5f, -1.5f, 0.0));
    unit->bounds.max = Vec3_Add(unit->position, Vec3_Create(1.5f, 1.5f, 6.0));
    
    short walkAnim = ANIM_BAT_WALK;
    short idleAnim = ANIM_BAT_IDLE;
    short attackAnim = ANIM_BAT_THROW;
    
    short dieAnim = ANIM_BAT_DIE;
    short hurtAnim = ANIM_BAT_HURT;
    
    Engine* engine = unit->engine;
    
    switch (unit->state)
    {
        case kUnitStateIdle:
        {
            // angle correction
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            }
            else
            {
                // randomize frame at the start
                const SkelAnim* idleSkelAnim = engine->renderSystem.anims + idleAnim;
                int startFrame = unit->skelModel.animator.anim ? 0 : SkelAnim_RandomFrame(idleSkelAnim);
                SkelAnimator_SetAnim(&unit->skelModel.animator, idleSkelAnim, startFrame, 12);
            }
            
            SkelModel_Tick(&unit->skelModel);

            break;
        }
        case kUnitStateDead:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[dieAnim], 0, 4);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) Unit_Kill(unit);
            break;
        }
        case kUnitStateHurt:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[hurtAnim], 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) unit->state = kUnitStateIdle;
            break;
        }
        case kUnitStateMove:
        {
            if (!Unit_FollowPath(unit)) unit->state = kUnitStateIdle;
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            
            if (frameInfo.marker != NULL)
            {
                if (strcmp(frameInfo.marker->name, "step1") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP1);
                }
                else if (strcmp(frameInfo.marker->name, "step2") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP2);
                }
            }
            break;
        }
        case kUnitStateAttackPrimary:
        case kUnitStateAttackSecondary:
        {
            const WeaponInfo* weapon = engine->weaponTable + unit->primaryWeapon;
            
            Vec3 dir = Vec3_Norm(Vec3_Sub(engine->command.position, unit->position));
            unit->targetAngle = RAD_TO_DEG(atan2(dir.y, dir.x));
            
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            
            // if we aren't facing the target, we need to rotate
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
                SkelModel_Tick(&unit->skelModel);
            }
            else
            {
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[attackAnim], 0, 6);
                SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

                if (frameInfo.finishedAnim)
                {
                    unit->actionCounter--;
                    unit->state = kUnitStateIdle;
                }
                else if (frameInfo.marker != NULL)
                {
                    const SkelAttachPoint* attachPoint = SkelModel_AttachPointAt(&unit->skelModel, 0);
                    
                    Vec3 emitPoint = Vec3_Add(attachPoint->modelPosition, Quat_MultVec3(&attachPoint->modelRotation, weapon->projectileOffset));
                    emitPoint = Vec3_Add(emitPoint, unit->position);
                    Vec3 endPoint = Vec3_Add(engine->command.position, Vec3_Sub(emitPoint, unit->position));
                    
                    Vec3 direction = Vec3_Norm(Vec3_Sub(endPoint, emitPoint));
                    direction = WeaponInfo_CalcRandSpread(weapon->spread, direction);
                    
                    Quat rotation = Quat_CreateLook(direction, Vec3_Create(1.0f, 0.0f, 0.0f));
                    
                    Prop* projectile = SceneSystem_SpawnPropAt(&engine->sceneSystem, weapon->projectilePropType, emitPoint, rotation, 0);
                    projectile->owner = unit;
                    projectile->hp = weapon->damage;
                    
                    if (rand() % weapon->critChance == 1)
                    {
                        projectile->hp += weapon->critBonus;
                    }
                }
            }
        }
        default: break;
    }
}


static void Vamp_OnSpawn(Unit* unit, int flags)
{
    Engine* engine = unit->engine;
    
    unit->onTick = Vamp_OnTick;
    unit->onSelect = Bat_OnSelect;
    unit->onStartPath = Bat_OnStartPath;
    unit->onKill = Vamp_OnKill;
    unit->onDamage = Vamp_OnDamage;
    unit->moveRange = 30;
    unit->viewRadius = 60;
    
    unit->maxHp = 30;
    unit->hp = unit->maxHp;
    
    Unit_SetName(unit, "Vampire");
    
    unit->primaryWeapon = kWeaponVampBall;
    
    unit->speed = 0.2f;
        
    SkelModel_Copy(&unit->skelModel, &engine->renderSystem.skelModels[SKEL_BAT]);
    unit->skelModel.material.diffuseMap = TEX_VAMP;
    
    unit->skelModel.attachPointTable[0] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "r_hand");
    unit->skelModel.attachPointTable[1] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "l_hand");
}

static void Wolf_OnSelect(Unit* unit)
{
    int sounds[] = {
        SND_WOLF_SELECT1,
        SND_WOLF_SELECT2,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }
    
    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}

static void Wolf_OnStartPath(Unit* unit)
{
    int sounds[] = {
        SND_WOLF_MOVE1,
        SND_WOLF_MOVE2,
    };
    
    int count = sizeof(sounds) / sizeof(int);
    
    for (int i = 0; i < count; ++i)
    {
        if (SndSystem_IsPlaying(&unit->engine->soundSystem, sounds[i]))
            return;
    }
    
    SndSystem_PlaySound(&unit->engine->soundSystem, sounds[rand() % count]);
}

static void Wolf_OnDamage(struct Unit* unit,
                         struct Prop* prop,
                         int damage,
                         DamageType damageType)
{
    if (unit->state == kUnitStateDead)
        return;
    
    Engine* engine = unit->engine;
    
    int defensePowerup = unit->powerups & kUnitPowerupDefense;
    
    switch (damageType)
    {
        case kDamageTypeMelee:
        {
            unit->isAlerted = 1;
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.3f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_WOLF_HURT1 : SND_WOLF_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeBullet:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.1f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_WOLF_HURT1 : SND_WOLF_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMagic:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.1f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_WOLF_HURT1 : SND_WOLF_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeAcid:
        {
            unit->isAlerted = 1;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, AABB_Center(unit->bounds), Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_WOLF_HURT1 : SND_WOLF_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMindControl:
        {
            unit->isAlerted = 1;
            
            int ownersUnitCount = engine->players[prop->owner->playerId]->unitCount;
            int diceRoll = (rand() % 3 == 1);
            
            if (diceRoll && ownersUnitCount < OBSERVERS_MAX)
            {
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectMindControl, unit->position, Quat_Identity, 0);
                SndSystem_PlaySound(&engine->soundSystem, SND_MIND_CONTROL);
                
                unit->playerId = prop->owner->playerId;
                ++engine->players[unit->playerId]->unitCount;
            }
            else
            {
                if (defensePowerup)
                    damage /= 2;
                
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
                
                SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_WOLF_HURT1 : SND_WOLF_HURT2);
                unit->state = kUnitStateHurt;
                unit->hp -= damage;
            }
            break;
        }
        case kDamageTypeHeal:
        {
            unit->hp += damage;
            
            if (unit->hp > unit->maxHp)
                unit->hp = unit->maxHp;
            break;
        }
        case kDamageTypeOther:
            unit->hp -= damage;
            break;
    }
    
    if (prop && prop->owner)
    {
        Vec2 ownerPoint2D = Vec2_FromVec3(prop->owner->position);
        float angleToOwner = RAD_TO_DEG(atan2f(ownerPoint2D.y - unit->position.y, ownerPoint2D.x - unit->position.x));
        unit->targetAngle = angleToOwner;
    }
    
    if (unit->hp < 1)
        unit->state = kUnitStateDead;
}


static void Wolf_OnTick(Unit* unit)
{
    unit->bounds = AABB_CreateAnchored(unit->position, Vec3_Create(3.0f, 3.0f, 5.5), Vec3_Create(0.5f, 0.5f, 0.0f));
    
    short walkAnim = ANIM_WOLF_WALK;
    short idleAnim = ANIM_WOLF_IDLE;
    short attackAnim = ANIM_WOLF_ATTACK;
    short dieAnim = ANIM_WOLF_DIE;
    short hurtAnim = ANIM_WOLF_HURT;
    
    Engine* engine = unit->engine;
    
    switch (unit->state)
    {
        case kUnitStateIdle:
        {
            // angle correction
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            }
            else
            {
                // randomize frame at the start
                const SkelAnim* idleSkelAnim = engine->renderSystem.anims + idleAnim;
                int startFrame = unit->skelModel.animator.anim ? 0 : SkelAnim_RandomFrame(idleSkelAnim);
                SkelAnimator_SetAnim(&unit->skelModel.animator, idleSkelAnim, startFrame, 12);
            }
            
            SkelModel_Tick(&unit->skelModel);
            break;
        }
        case kUnitStateDead:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[dieAnim], 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) Unit_Kill(unit);
            break;
        }
        case kUnitStateHurt:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[hurtAnim], 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) unit->state = kUnitStateIdle;
            break;
        }
        case kUnitStateMove:
        {
            if (!Unit_FollowPath(unit)) unit->state = kUnitStateIdle;
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            
            if (frameInfo.marker != NULL)
            {
                if (strcmp(frameInfo.marker->name, "step1") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP1);
                }
                else if (strcmp(frameInfo.marker->name, "step2") == 0)
                {
                    SndSystem_PlaySound(&unit->engine->soundSystem, SND_STEP2);
                }
            }
            break;
        }
        case kUnitStateAttackPrimary:
        case kUnitStateAttackSecondary:
        {
            const WeaponInfo* weapon = engine->weaponTable + unit->primaryWeapon;
            
            Vec3 dir = Vec3_Norm(Vec3_Sub(engine->command.position, unit->position));
            unit->targetAngle = RAD_TO_DEG(atan2(dir.y, dir.x));
            
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            
            // if we aren't facing the target, we need to rotate
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
                SkelModel_Tick(&unit->skelModel);
            }
            else
            {
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[attackAnim], 0, 6);
                SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

                if (frameInfo.finishedAnim)
                {
                    unit->actionCounter--;
                    unit->state = kUnitStateIdle;
                }
                else if (frameInfo.marker != NULL)
                {
                    const SkelAttachPoint* attachPoint = SkelModel_AttachPointAt(&unit->skelModel, 0);
                    
                    Vec3 emitPoint = Vec3_Add(attachPoint->modelPosition, unit->position);
                    
                    Prop* projectile = SceneSystem_SpawnPropAt(&engine->sceneSystem, weapon->projectilePropType, emitPoint, attachPoint->modelRotation, 0);
                    projectile->owner = unit;
                    projectile->hp = weapon->damage;
                    
                    SndSystem_PlaySound(&unit->engine->soundSystem, (rand() % 2 == 0) ? SND_WOLF_ATTACK1 : SND_WOLF_ATTACK2);
                }
            }
        }
        default: break;
    }

}

static void Wolf_OnKill(Unit* unit)
{
    SndSystem_PlaySound(&unit->engine->soundSystem, SND_WOLF_DIE);
    PartSystem_EmitEffect(&unit->engine->partSystem, kPartEffectSmoke, Vec3_Offset(unit->position, 0.0, 0.0, 1.0), Quat_Identity, 0);
}

static void Wolf_OnSpawn(Unit* unit, int flags)
{
    Engine* engine = unit->engine;
    
    unit->onTick = Wolf_OnTick;
    unit->onSelect = Wolf_OnSelect;
    unit->onStartPath = Wolf_OnStartPath;
    unit->onKill = Wolf_OnKill;
    unit->onDamage = Wolf_OnDamage;
    unit->moveRange = 65;
    unit->viewRadius = 75;
    unit->maxHp = 85;
    unit->hp = unit->maxHp;
    
    Unit_SetName(unit, "Wolf");
    
    unit->primaryWeapon = kWeaponWolfBite;
    
    unit->speed = 0.23f;
    
    SkelModel_Copy(&unit->skelModel, &engine->renderSystem.skelModels[SKEL_WOLF]);
    unit->skelModel.material.diffuseMap = TEX_WOLF;
    
    unit->skelModel.attachPointTable[0] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "attach_head");
}

static void Boss_OnStartPath(Unit* unit)
{
    int dice = rand() % 2;
    
    if (dice == 0)
    {
        SndSystem_PlaySound(&unit->engine->soundSystem, SND_BOSS_MOVE1);
    }
    else
    {
        SndSystem_PlaySound(&unit->engine->soundSystem, SND_BOSS_MOVE2);
    }
}

static void Boss_OnDamage(struct Unit* unit,
                          struct Prop* prop,
                          int damage,
                          DamageType damageType)
{
    if (unit->state == kUnitStateDead)
        return;
    
    Engine* engine = unit->engine;
    
    int defensePowerup = unit->powerups & kUnitPowerupDefense;
    
    switch (damageType)
    {
        case kDamageTypeMelee:
        {
            unit->isAlerted = 1;
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.5f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BOSS_HURT1 : SND_BOSS_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeBullet:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.5f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BOSS_HURT1 : SND_BOSS_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMagic:
        {
            unit->isAlerted = 1;
            
            float directionFactor = CalcDirectionFactor(unit->forward, prop->forward, 0.6f);
            damage *= directionFactor;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BOSS_HURT1 : SND_BOSS_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeAcid:
        {
            unit->isAlerted = 1;
            
            if (defensePowerup)
                damage /= 2;
            
            unit->hp -= damage ;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, AABB_Center(unit->bounds), Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BOSS_HURT1 : SND_BOSS_HURT2);
            unit->state = kUnitStateHurt;
            break;
        }
        case kDamageTypeMindControl:
        {
            unit->isAlerted = 1;
            
            if (defensePowerup)
                damage /= 2;
            
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, prop->position, Quat_Identity, 0);
            
            SndSystem_PlaySound(&unit->engine->soundSystem,  (rand() % 2 == 0) ? SND_BOSS_HURT1 : SND_BOSS_HURT2);
            unit->state = kUnitStateHurt;
            unit->hp -= damage;
            
            break;
        }
        case kDamageTypeHeal:
        {
            unit->hp += damage;
            
            if (unit->hp > unit->maxHp)
                unit->hp = unit->maxHp;
            
            break;
        }
        case kDamageTypeOther:
            unit->hp -= damage;
            break;
    }
    
    if (prop && prop->owner)
    {
        Vec2 ownerPoint2D = Vec2_FromVec3(prop->owner->position);
        float angleToOwner = RAD_TO_DEG(atan2f(ownerPoint2D.y - unit->position.y, ownerPoint2D.x - unit->position.x));
        unit->targetAngle = angleToOwner;
    }
    
    if (unit->hp < 1)
        unit->state = kUnitStateDead;
}

static void Boss_OnKill(Unit* unit)
{
    Vec3 p = Vec3_Add(unit->position, Vec3_Create(0.0f, 0.0f, 1.0f));
    SceneSystem_SpawnPropAt(&unit->engine->sceneSystem, kPropSkull, p, Quat_CreateAngle(unit->angle, 0.0f, 0.0f, 1.0f), 0);
    
    SndSystem_PlaySound(&unit->engine->soundSystem, SND_BOSS_DIE);
    PartSystem_EmitEffect(&unit->engine->partSystem, kPartEffectSmoke, Vec3_Offset(unit->position, 0.0, 0.0, 1.0), Quat_Identity, 0);
}

static void Boss_OnTick(Unit* unit)
{
    unit->bounds = AABB_CreateAnchored(unit->position, Vec3_Create(3.0f, 3.0f, 6.5f), Vec3_Create(0.5f, 0.5f, 0.0f));
    
    short walkAnim = ANIM_WITCH_WALK;
    short idleAnim = ANIM_WITCH_IDLE;
    short attackAnim = ANIM_WITCH_ATTACK;
    short dieAnim = ANIM_WITCH_DIE;
    short hurtAnim = ANIM_WITCH_HURT;
    short teleportAnim = ANIM_WITCH_TELEPORT;
    
    Engine* engine = unit->engine;
    
    switch (unit->state)
    {
        case kUnitStateIdle:
        {
            // angle correction
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            }
            else
            {
                // randomize frame at the start
                const SkelAnim* idleSkelAnim = engine->renderSystem.anims + idleAnim;
                int startFrame = unit->skelModel.animator.anim ? 0 : SkelAnim_RandomFrame(idleSkelAnim);
                SkelAnimator_SetAnim(&unit->skelModel.animator, idleSkelAnim, startFrame, 12);
            }
            
            SkelModel_Tick(&unit->skelModel);
            break;
        }
        case kUnitStateDead:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[dieAnim], 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) Unit_Kill(unit);
            break;
        }
        case kUnitStateHurt:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[hurtAnim], 0, 3);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);
            if (frameInfo.finishedAnim) unit->state = kUnitStateIdle;
            break;
        }
        case kUnitStateMove:
        {
            if (!Unit_FollowPath(unit)) unit->state = kUnitStateIdle;
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
            SkelModel_Tick(&unit->skelModel);
            break;
        }
        case kUnitStateTeleport:
        {
            SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[teleportAnim], 0, 6);
            SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

            if (frameInfo.marker != NULL)
            {
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectTeleport, AABB_Center(unit->bounds), Quat_Identity, 0);
                SndSystem_PlaySound(&engine->soundSystem, SND_BOSS_TELEPORT);
            }
            else if (frameInfo.finishedAnim)
            {
                unit->state = kUnitStateIdle;
                
                unit->position = engine->command.position;
                PartSystem_EmitEffect(&engine->partSystem, kPartEffectSpawn, unit->position, Quat_CreateAngle(unit->angle, 0.0f, 0.0f, 1.0f), 0);
                SndSystem_PlaySound(&engine->soundSystem, SND_SPAWN);
                unit->actionCounter--;
            }
            break;
        }
        case kUnitStateAttackPrimary:
        case kUnitStateAttackSecondary:
        {
            const WeaponInfo* weapon = engine->weaponTable + unit->primaryWeapon;
            
            Vec3 dir = Vec3_Norm(Vec3_Sub(engine->command.position, unit->position));
            unit->targetAngle = RAD_TO_DEG(atan2(dir.y, dir.x));
            
            float angleDiff = Deg_Diff(unit->targetAngle, unit->angle);
            
            // if we aren't facing the target, we need to rotate
            if (fabs(angleDiff) > UNIT_ANGLE_SPEED)
            {
                angleDiff = CLAMP(angleDiff, -UNIT_ANGLE_SPEED, UNIT_ANGLE_SPEED);
                unit->angle = Deg_Normalize(unit->angle + angleDiff);
                
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[walkAnim], 0, 6);
                SkelModel_Tick(&unit->skelModel);
            }
            else
            {
                SkelAnimator_SetAnim(&unit->skelModel.animator,  &engine->renderSystem.anims[attackAnim], 0, 6);
                SkelAnimatorInfo frameInfo = SkelModel_Tick(&unit->skelModel);

                if (frameInfo.finishedAnim)
                {
                    unit->actionCounter--;
                    unit->state = kUnitStateIdle;
                }
                else if (frameInfo.marker != NULL)
                {
                    if (unit->state == kUnitStateAttackPrimary)
                    {
                        const SkelAttachPoint* attachPoint = SkelModel_AttachPointAt(&unit->skelModel, 0);
                        
                        Vec3 emitPoint = Vec3_Add(attachPoint->modelPosition, Quat_MultVec3(&attachPoint->modelRotation, weapon->projectileOffset));
                        emitPoint = Vec3_Add(emitPoint, unit->position);
                        Vec3 endPoint = Vec3_Add(engine->command.position, Vec3_Sub(emitPoint, unit->position));
                        
                        Vec3 direction = Vec3_Norm(Vec3_Sub(endPoint, emitPoint));
                        direction = WeaponInfo_CalcRandSpread(weapon->spread, direction);
                        
                        Quat rotation = Quat_CreateLook(direction, Vec3_Create(1.0f, 0.0f, 0.0f));
                        
                        Prop* projectile = SceneSystem_SpawnPropAt(&engine->sceneSystem, weapon->projectilePropType, emitPoint, rotation, 0);
                        projectile->owner = unit;
                        projectile->target = endPoint;
                        projectile->hp = weapon->damage;
                        
                        if (rand() % weapon->critChance == 1)
                        {
                            projectile->hp += weapon->critBonus;
                        }
                    }
                    else
                    {
                        float faceAngle = (float)(rand() % 360);
                        
                        int bossLevel = unit->user1;
                        int spawnType = kUnitBat;
                        
                        if (bossLevel >= 1 && rand() % 4 == 3)
                        {
                            spawnType = kUnitVamp;
                        }
                        else if (bossLevel >= 2 && rand() % 4 == 2)
                        {
                            spawnType = kUnitPhantom;
                        }
                        else if (bossLevel >= 3 && rand() % 4 == 1)
                        {
                            spawnType = kUnitWolf;
                        }
                        
                        Unit* spawned = SceneSystem_SpawnUnitAt(&engine->sceneSystem, spawnType, engine->command.position, faceAngle, 0);
                        spawned->playerId = 1;
                        spawned->isAlerted = 1;
                        PartSystem_EmitEffect(&engine->partSystem, kPartEffectSpawn, spawned->position, Quat_CreateAngle(faceAngle, 0.0f, 0.0f, 1.0f), 0);
                        SndSystem_PlaySound(&engine->soundSystem, SND_SPAWN_ENEMY);
                    }
                }
            }
        }
        default: break;
    }
}

static void Boss_OnSpawn(Unit* unit, int flags)
{
    Engine* engine = unit->engine;
    
    unit->onTick = Boss_OnTick;
    unit->onStartPath = Boss_OnStartPath;
    unit->onKill = Boss_OnKill;
    unit->onDamage = Boss_OnDamage;
    unit->maxHp = 210;
    unit->hp = unit->maxHp;
    unit->moveRange = 45;
    unit->viewRadius = 60;
    
    /* boss level determines enemies spawned */
    unit->user1 = 1;
    
    unit->powerups = kUnitPowerupDefense;
    
    Unit_SetName(unit, "Boss");
    
    unit->primaryWeapon = kWeaponMagicBall;
    
    unit->speed = 0.24f;
    
    SkelModel_Copy(&unit->skelModel, &engine->renderSystem.skelModels[SKEL_WITCH]);
    unit->skelModel.material.diffuseMap = TEX_WITCH;

    unit->skelModel.attachPointTable[0] = Skel_FindAttachPointIndex(&unit->skelModel.skel, "attach_magic");
}

static void Skull_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (unit->playerId == ENGINE_PLAYER_LOCAL)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_SKULL_PICKUP);
        ++unit->engine->players[unit->playerId]->skulls;
        Prop_Kill(prop);
    }
}

static void Skull_OnTick(Prop* prop)
{
    prop->rotation = Quat_Mult(prop->rotation, Quat_CreateAngle(2.0f, 0.0f, 0.0f, 1.0f));
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.5f, 1.5f, 1.5f));
}

static void Skull_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = Skull_OnTick;
    prop->onTouchUnit = Skull_OnTouchUnit;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_SKULL);
    prop->model.material.diffuseMap = TEX_SKULL;
    prop->touchEnabled = 1;
}

static void MindDamage_OnTouch(Prop* prop, Prop* other)
{
    if (other->onDamage)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_SYRINGE_HIT);
        Prop_Damage(other, prop, prop->hp, kDamageTypeMindControl);
        Prop_Kill(prop);
    }
}

static void MindDamage_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (prop->owner == unit || unit->state == kUnitStateDead)
        return;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_SYRINGE_HIT);
    Unit_Damage(unit, prop, prop->hp, kDamageTypeMindControl);
    Prop_Kill(prop);
}

static void MindDamage_OnTick(Prop* prop)
{
    if (--prop->timer <= 0)
        Prop_Kill(prop);
}

static void MindDamage_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = MindDamage_OnTick;
    prop->onTouch = MindDamage_OnTouch;
    prop->onTouchUnit = MindDamage_OnTouchUnit;
    prop->touchEnabled = 1;
    prop->visible = 0;
    
    prop->timer = 4;
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(3.0f, 3.0f, 5.0f));
}

static void MeleeDamage_OnTouch(Prop* prop, Prop* other)
{
    if (other->onDamage)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_MELEE_HIT);
        Prop_Damage(other, prop, prop->hp, kDamageTypeMelee);
        Prop_Kill(prop);
    }
}

static void MeleeDamage_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (prop->owner == unit || unit->state == kUnitStateDead)
        return;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MELEE_HIT);
    Unit_Damage(unit, prop, prop->hp, kDamageTypeMelee);
    Prop_Kill(prop);
}

static void MeleeDamage_OnTick(Prop* prop)
{
    if (--prop->timer <= 0)
        Prop_Kill(prop);
}

static void MeleeDamage_OnSpawn(Prop* prop, int flags)
{
    if (!SndSystem_IsPlaying(&prop->engine->soundSystem, SND_MELEE))
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_MELEE);

    prop->onTick = MeleeDamage_OnTick;
    prop->onTouch = MeleeDamage_OnTouch;
    prop->onTouchUnit = MeleeDamage_OnTouchUnit;
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(2.0f, 2.0f, 5.0f));

    prop->visible = 0;
    prop->touchEnabled = 1;
    prop->timer = 3;
}

static void CannonBullet_OnTouch(Prop* prop, Prop* other)
{
    if (other->onDamage)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_CANNON_HIT);
        Prop_Damage(other, prop, prop->hp * prop->speed, kDamageTypeBullet);
        Prop_Kill(prop);
    }
}

static void CannonBullet_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (prop->owner == unit || unit->state == kUnitStateDead)
        return;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_CANNON_HIT);
    Unit_Damage(unit, prop, prop->hp * (prop->speed / 1.5f), kDamageTypeBullet);
    Prop_Kill(prop);
}

static void CannonBullet_OnTick(Prop* prop)
{
    if (--prop->timer <= 0)
        Prop_Kill(prop);
    
    if (prop->speed < 1.5f)
    {
        prop->speed *= 1.054f;
    }
    else
    {
        prop->speed = 1.5f;
    }
    
    Vec3 move = Vec3_Scale(prop->forward, prop->speed);
    
    Vec3 oldPosition = prop->position;
    Vec3 newPosition = Vec3_Add(prop->position, move);
    
    if (NavSystem_LineIntersectsSolid(&prop->engine->navSystem, oldPosition, newPosition, 10.0f))
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_CANNON_HIT);
        Prop_Kill(prop);
    }
    
    prop->rotation = Quat_CreateAngle(RAD_TO_DEG(atan2f(prop->forward.x, -prop->forward.y)) - 90.0f, 0.0f, 0.0f, 1.0f);
    prop->position = newPosition;
    prop->bounds = AABB_CreateCentered(newPosition, Vec3_Create(1.0f, 1.0f, 1.0f));
}

static void CannonBullet_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = CannonBullet_OnTick;
    prop->onTouch = CannonBullet_OnTouch;
    prop->onTouchUnit = CannonBullet_OnTouchUnit;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_CANNON_BULLET);
    prop->model.material.diffuseMap = TEX_CANNON_BULLET;
    prop->model.material.flags = kMaterialFlagUnlit;
    
    prop->touchEnabled = 1;
    prop->speed = 0.035f;
    
    prop->timer = 90;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_CANNON_BULLET);
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.0f, 1.0f, 1.0f));
}

static void MgBullet_OnTouch(Prop* prop, Prop* other)
{
    if (other->onDamage)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_MG_BULLET_HIT);
        Prop_Damage(other, prop, prop->hp, kDamageTypeBullet);
        Prop_Kill(prop);
    }
}

static void MgBullet_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (prop->owner == unit || unit->state == kUnitStateDead)
        return;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MG_BULLET_HIT);
    Unit_Damage(unit, prop, prop->hp, kDamageTypeBullet);
    Prop_Kill(prop);
}

static void MgBullet_OnTick(Prop* prop)
{
    if (prop->hp > 8)
        --prop->hp;
    
    if (--prop->timer <= 0)
        Prop_Kill(prop);
    
    Vec3 move = Vec3_Scale(prop->forward, 1.7f);

    Vec3 oldPosition = prop->position;
    Vec3 newPosition = Vec3_Add(prop->position, move);
    
    if (NavSystem_LineIntersectsSolid(&prop->engine->navSystem, oldPosition, newPosition, 10.0f))
        Prop_Kill(prop);
    
    prop->position = newPosition;
    prop->bounds = AABB_CreateCentered(newPosition, Vec3_Create(1.0f, 1.0f, 1.0f));
}

static void MgBullet_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = MgBullet_OnTick;
    prop->onTouch = MgBullet_OnTouch;
    prop->onTouchUnit = MgBullet_OnTouchUnit;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_MG_BULLET);
    prop->model.material.diffuseMap = TEX_MG_BULLET;
    prop->model.material.flags = kMaterialFlagUnlit;
    
    prop->touchEnabled = 1;
    prop->timer = 24;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MG_BULLET);
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.0f, 1.0f, 1.0f));
}

static void RevolverBullet_OnTouch(Prop* prop, Prop* other)
{
    if (other->onDamage)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_MG_BULLET_HIT);
        Prop_Damage(other, prop, prop->hp, kDamageTypeBullet);
        Prop_Kill(prop);
    }
}

static void RevolverBullet_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (prop->owner == unit || unit->state == kUnitStateDead)
        return;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MG_BULLET_HIT);
    Unit_Damage(unit, prop, prop->hp, kDamageTypeBullet);
    Prop_Kill(prop);
}

static void RevolverBullet_OnTick(Prop* prop)
{
    if (--prop->timer <= 0)
        Prop_Kill(prop);
    
    Vec3 move = Vec3_Scale(prop->forward, 1.7f);
    
    Vec3 oldPosition = prop->position;
    Vec3 newPosition = Vec3_Add(prop->position, move);
    
    if (NavSystem_LineIntersectsSolid(&prop->engine->navSystem, oldPosition, newPosition, 10.0f))
        Prop_Kill(prop);
    
    prop->position = newPosition;
    prop->bounds = AABB_CreateCentered(newPosition, Vec3_Create(1.0f, 1.0f, 1.0f));
}

static void RevolverBullet_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = RevolverBullet_OnTick;
    prop->onTouch = RevolverBullet_OnTouch;
    prop->onTouchUnit = RevolverBullet_OnTouchUnit;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_MG_BULLET);
    prop->model.material.diffuseMap = TEX_MG_BULLET;
    prop->model.material.flags = kMaterialFlagUnlit;
    
    prop->touchEnabled = 1;
    prop->timer = 35;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MG_BULLET);
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.0f, 1.0f, 1.0f));
}

static void VampBall_OnTouch(Prop* prop, Prop* other)
{
    if (other->onDamage)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_MAGIC_HIT);
        Prop_Damage(other, prop, prop->hp, kDamageTypeMagic);
        Prop_Kill(prop);
    }
}

static void VampBall_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (prop->owner == unit || unit->state == kUnitStateDead)
        return;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MAGIC_HIT);
    Unit_Damage(unit, prop, prop->hp, kDamageTypeMagic);
    Prop_Kill(prop);
}

static void VampBall_OnTick(Prop* prop)
{
    if (--prop->timer <= 0)
        Prop_Kill(prop);
    
    Vec3 move = Vec3_Scale(prop->forward, 0.75f);
    
    Vec3 oldPosition = prop->position;
    Vec3 newPosition = Vec3_Add(prop->position, move);
    
    if (NavSystem_LineIntersectsSolid(&prop->engine->navSystem, oldPosition, newPosition, 10.0f))
        Prop_Kill(prop);
    
    prop->position = newPosition;
    prop->bounds = AABB_CreateCentered(newPosition, Vec3_Create(1.0f, 1.0f, 1.0f));
    
    int partEmitterIndex = prop->data[0];
    
    if (partEmitterIndex != -1)
    {
        PartEmitter* emitter = prop->engine->partSystem.emitters + partEmitterIndex;
        emitter->position = newPosition;
    }
}

static void VampBall_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = VampBall_OnTick;
    prop->onTouch = VampBall_OnTouch;
    prop->onTouchUnit = VampBall_OnTouchUnit;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_MAGIC_BALL);
    prop->model.material.diffuseMap = TEX_VAMP_BALL;
    prop->model.material.flags = kMaterialFlagUnlit;

    prop->touchEnabled = 1;
    prop->timer = 60;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MAGIC_BALL);
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.0f, 1.0f, 1.0f));
    
    PartEmitter* emitter = PartSystem_EmitEffect(&prop->engine->partSystem, kPartEffectRedMagic, prop->position, prop->rotation, 0);
    
    if (emitter)
    {
        emitter->controlPoints[0] = prop->position;
        prop->data[0] = emitter->index;
    }
}


static void MagicBall_OnTouch(Prop* prop, Prop* other)
{
    if (other->onDamage)
    {
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_MAGIC_HIT);
        Prop_Damage(other, prop, prop->hp, kDamageTypeMagic);
        Prop_Kill(prop);
    }
}

static void MagicBall_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (prop->owner == unit || unit->state == kUnitStateDead)
        return;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MAGIC_HIT);
    Unit_Damage(unit, prop, prop->hp, kDamageTypeMagic);
    Prop_Kill(prop);
}

static void MagicBall_OnTick(Prop* prop)
{
    if (--prop->timer <= 0)
        Prop_Kill(prop);
    
    Vec3 move = Vec3_Scale(prop->forward, 1.0);
    
    Vec3 oldPosition = prop->position;
    Vec3 newPosition = Vec3_Add(prop->position, move);
    
    if (NavSystem_LineIntersectsSolid(&prop->engine->navSystem, oldPosition, newPosition, 10.0f))
        Prop_Kill(prop);

    prop->position = newPosition;
    prop->bounds = AABB_CreateCentered(newPosition, Vec3_Create(1.0f, 1.0f, 1.0f));

    int partEmitterIndex = prop->data[0];
    
    if (partEmitterIndex != -1)
    {
        PartEmitter* emitter = prop->engine->partSystem.emitters + partEmitterIndex;
        emitter->position = newPosition;
    }
}

static void MagicBall_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = MagicBall_OnTick;
    prop->onTouch = MagicBall_OnTouch;
    prop->onTouchUnit = MagicBall_OnTouchUnit;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_MAGIC_BALL);
    prop->model.material.diffuseMap = TEX_MAGIC_BALL;
    prop->model.material.flags = kMaterialFlagUnlit;
    
    prop->touchEnabled = 1;
    prop->timer = 60;
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_MAGIC_BALL);
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.0f, 1.0f, 1.0f));
    
    PartEmitter* emitter = PartSystem_EmitEffect(&prop->engine->partSystem, kPartEffectPurpleMagic, prop->position, prop->rotation, 0);
    
    if (emitter)
    {
        emitter->controlPoints[0] = prop->position;
        prop->data[0] = emitter->index;
    }
}

static void Weapon_OnTick(Prop* prop)
{
    // this box doesn't affect gameplay but is just for camera visibility
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(6.0f, 6.0f, 2.0f));
}

static void Weapon_OnSpawn(Prop* prop, int flags)
{
    prop->onTick = Weapon_OnTick;
}

static void UnitSpawn_OnVar(Prop* prop, const char* key, const char* val)
{
    if (strcmp(key, "unit") == 0)
    {
        UnitType unitType = kUnitNone;
        
        if (strcmp(val, "SCIENTIST") == 0)
        {
            unitType = kUnitScientist;
        }
        else if (strcmp(val, "BAT") == 0)
        {
            unitType = kUnitBat;
        }
        else if (strcmp(val, "VAMP") == 0)
        {
            unitType = kUnitVamp;
        }
        else if (strcmp(val, "WOLF") == 0)
        {
            unitType = kUnitWolf;
        }
        else if (strcmp(val, "PHANTOM") == 0)
        {
            unitType = kUnitPhantom;
        }
        else if (strcmp(val, "BOSS") == 0)
        {
            unitType = kUnitBoss;
        }
        else
        {
            assert(0);
        }
        prop->data[0] = unitType;
    }
    else if (strcmp(key, "player") == 0)
    {
        int playerId = atoi(val);
        prop->data[1] = playerId;
    }
    else if (strcmp(key, "index") == 0)
    {
        int index = atoi(val);
        prop->data[2] = index;
    }
    else if (strcmp(key, "inactive") == 0)
    {
        /* whether the spawn should be inactive (such as for triggers) */
        prop->inactive = atoi(val);
    }
    else if (strcmp(key, "effect") == 0)
    {
        /* whether to show a particle effect or not */
        prop->data[3] = atoi(val);
    }
    else if (strcmp(key, "level") == 0)
    {
        /* determines power of enemies spawned */
        prop->data[4] = atoi(val);
    }
    else if (strcmp(key, "is_alerted") == 0)
    {
        prop->data[5] = atoi(val);
    }
    else if (strcmp(key, "hp") == 0)
    {
        prop->hp = atoi(val);
    }
}

static void UnitSpawn_OnTick(Prop* prop)
{
    Engine* engine = prop->engine;
    
    UnitType unitType = (UnitType)prop->data[0];
    int playerId = prop->data[1];
    int unitSpawnIndex = prop->data[2];
    
    if (strcmp(prop->identifier, "spawn_bat.003") == 0)
    {
        printf("danger\n");
    }
    
    
    if (unitSpawnIndex != -1 && unitSpawnIndex >= engine->players[playerId]->unitSpawnInfoCount)
    {
        // invalid count;
        prop->dead = 1;
        return;
    }
    
    if (unitSpawnIndex != -1)
    {
        const UnitInfo* unitInfo = engine->players[playerId]->unitSpawnInfo + unitSpawnIndex;
        unitType = unitInfo->type;
    }
    
    float yaw = RAD_TO_DEG(prop->spawnRotation.z);
    Unit* unit = SceneSystem_SpawnUnitAt(&prop->engine->sceneSystem, unitType, prop->position, Deg_Normalize(yaw), 0);
    unit->playerId = playerId;
    
    if (unitSpawnIndex != -1)
    {
        const UnitInfo* unitInfo = engine->players[playerId]->unitSpawnInfo + unitSpawnIndex;
        Unit_SetName(unit, unitInfo->name);
        unit->primaryWeapon = unitInfo->primaryWeapon;
        unit->secondaryWeapon = unitInfo->secondaryWeapon;
        unit->crewIndex = unitInfo->crewIndex;
    }
    
    int showPartEffect = (prop->data[3] == 1);
    if (showPartEffect)
    {
        PartSystem_EmitEffect(&engine->partSystem, kPartEffectSpawn, unit->position, Quat_CreateAngle(unit->angle, 0.0f, 0.0f, 1.0f), 0);
        
        if (!SndSystem_IsPlaying(&engine->soundSystem, SND_SPAWN))
            SndSystem_PlaySound(&engine->soundSystem, SND_SPAWN);
    }
    
    if (unitType == kUnitBoss)
    {
        int bossLevel = prop->data[4];
        unit->user1 = bossLevel;
    }
    
    int isAlerted = prop->data[5];
    
    if (isAlerted != -1)
    {
        unit->isAlerted = isAlerted;
    }
    
    if (prop->hp != -1)
    {
        unit->hp = prop->hp;
    }
    
    /* only one time */
    prop->inactive = 1;
    //prop->dead = 1;
}

static void UnitSpawn_OnSpawn(Prop* prop, int flags)
{
    prop->visible = 0;
    prop->onVar = UnitSpawn_OnVar;
    prop->onTick = UnitSpawn_OnTick;
    
    prop->hp = -1;
}

static void ItemPickup_OnTick(Prop* prop)
{
    prop->rotation = Quat_Mult(prop->rotation, Quat_CreateAngle(1.2f, 0.0f, 0.0f, 1.0f));
}

static void ItemPickup_OnVar(Prop* prop, const char* key, const char* val)
{
    if (strcmp(key, "item") == 0)
    {
        int model = -1;
        int texture = -1;
        
        if (strcmp(val, "HP_KIT") == 0)
        {
            model = MODEL_HP_KIT;
            texture = TEX_HP_KIT;
        }
        else if (strcmp(val, "DEFENSE_KIT") == 0)
        {
            model = MODEL_DEFENSE_KIT;
            texture = TEX_DEFENSE_KIT;
        }
        else if (strcmp(val, "RANGE_KIT") == 0)
        {
            model = MODEL_RANGE_KIT;
            texture = TEX_RANGE_KIT;
        }
        
        assert(model != -1);
        assert(texture != -1);
        StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + model);
        prop->model.material.diffuseMap = texture;
        
        prop->data[0] = model;
    }
}

static void ItemPickup_OnAction(Prop* prop, Unit* unit)
{    
    int model = prop->data[0];
    
    if (model == MODEL_HP_KIT)
    {
        unit->hp = MIN(unit->hp + 40, UNIT_HP_MAX);
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_HEAL);
        
    }
    else if (model == MODEL_DEFENSE_KIT)
    {
        if (unit->powerups & kUnitPowerupDefense)
            return; // prevent 'doubling up' on bonus
        
        unit->powerups |= kUnitPowerupDefense;
    }
    else if (model == MODEL_RANGE_KIT)
    {
        if (unit->powerups & kUnitPowerupRange)
            return;  // prevent 'doubling up' on bonus
        
        unit->powerups |= kUnitPowerupRange;
    }
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_WEAPON_PICKUP);
    prop->dead = 1;
}

static void ItemPickup_OnSpawn(Prop* prop, int flags)
{
    prop->touchEnabled = 1;
    prop->onTick = ItemPickup_OnTick;
    prop->onAction = ItemPickup_OnAction;
    prop->onVar = ItemPickup_OnVar;
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.5f, 1.5f, 2.0));
}

static void WeaponPickup_OnTick(Prop* prop)
{
    prop->rotation = Quat_Mult(prop->rotation, Quat_CreateAngle(1.2f, 1.0f, 0.0f, 0.0f));
}

static void WeaponPickup_OnVar(Prop* prop, const char* key, const char* val)
{
    if (strcmp(key, "weapon") == 0)
    {
        int model = -1;
        int texture = -1;
        
        if (strcmp(val, "REVOLVER") == 0)
        {
            prop->data[0] = kWeaponRevolver;
            model = MODEL_REVOLVER;
            texture = TEX_REVOLVER;
        }
        else if (strcmp(val, "CANNON") == 0)
        {
            prop->data[0] = kWeaponCannon;
            model = MODEL_CANNON;
            texture = TEX_CANNON;
        }
        else if (strcmp(val, "MG") == 0)
        {
            prop->data[0] = kWeaponMg;
            model = MODEL_MG;
            texture = TEX_MG;
        }
        else if (strcmp(val, "SYRINGE") == 0)
        {
            prop->data[0] = kWeaponSyringe;
            model = MODEL_SYRINGE;
            texture = TEX_SYRINGE;
        }
        else if (strcmp(val, "MACHETE") == 0)
        {
            prop->data[0] = kWeaponMachete;
            model = MODEL_MACHETE;
            texture = TEX_MACHETE;
        }
        
        assert(model != -1);
        assert(texture != -1);
        StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + model);
        prop->model.material.diffuseMap = texture;
    }
}

static void WeaponPickup_OnAction(Prop* prop, Unit* unit)
{
    if (unit->type == kUnitScientist)
    {
        Player* player = unit->engine->players[unit->playerId];
        
        if (prop->data[0] == kWeaponMachete)
        {
            // Machete is a special case because it is the only secondary weapon
            unit->secondaryWeapon = prop->data[0];
        }
        else
        {
            unit->primaryWeapon = prop->data[0];
        }
        
        Player_Select(player, unit);
        
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_WEAPON_PICKUP);
        prop->dead = 1;
    }
}

static void WeaponPickup_OnSpawn(Prop* prop, int flags)
{
    prop->touchEnabled = 1;
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(1.5f, 1.5f, 2.0));
    prop->onTick = WeaponPickup_OnTick;
    prop->onAction = WeaponPickup_OnAction;
    prop->onVar = WeaponPickup_OnVar;
    prop->rotation = Quat_CreateAngle(-90.0f, 0.0f, 1.0f, 0.0f);
}

static void Button_OnAction(Prop* prop, Unit* unit)
{
    Prop_InputEvent(prop, prop, kEventAction, 0);
}

static void Button_OnSpawn(Prop* prop, int flags)
{
    prop->visible = 0;
    prop->touchEnabled = 1;
    prop->onAction = Button_OnAction;
}

static void Trigger_OnVar(Prop* prop, const char* key, const char* val)
{
    if (strcmp(key, "player") == 0)
    {
        prop->data[0] = atoi(val);
    }
}

static void Trigger_OnTouchUnit(Prop* prop, Unit* unit)
{
    int playerId = prop->data[0];
    
    if (playerId == unit->playerId)
    {
        Prop_InputEvent(prop, NULL, kEventActivate, unit->index);
        prop->inactive = 1;
    }
}

static void Trigger_OnSpawn(Prop* prop, int flags)
{
    prop->visible = 0;
    prop->touchEnabled = 1;
    prop->onVar = Trigger_OnVar;
    prop->onTouchUnit = Trigger_OnTouchUnit;
}

static void EggHealer_OnDamage(struct Prop* prop, struct Prop* other, int damage, DamageType damageType)
{
    Engine* engine = prop->engine;
    
    switch (damageType)
    {
        case kDamageTypeMelee:
        {
            prop->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, other->position, Quat_Identity, 0);
            break;
        }
        case kDamageTypeBullet:
        {
            prop->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, other->position, Quat_Identity, 0);
            break;
        }
        case kDamageTypeMagic:
        {
            prop->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, other->position, Quat_Identity, 0);
            break;
        }
        case kDamageTypeAcid:
        {
            float radius = AABB_Size(other->bounds).x / 2.0f;
            float distanceFactor = Vec3_Dist(prop->position, other->position) / radius;
            
            prop->hp -= damage * distanceFactor;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, AABB_Center(prop->bounds), Quat_Identity, 0);
            break;
        }
        case kDamageTypeMindControl:
        {
            prop->hp -= damage;
            PartSystem_EmitEffect(&engine->partSystem, kPartEffectBlood, other->position, Quat_Identity, 0);
            break;
        }
        case kDamageTypeHeal:
        {
            break;
        }
        case kDamageTypeOther:
        {
            prop->hp -= damage;
            break;
        }
    }

    if (prop->hp < 0)
    {
        prop->touchEnabled = 0;
        prop->onEvent = NULL;
        prop->onTouch = NULL;
        prop->onDamage = NULL;
        
        StaticModel_Shutdown(&prop->model);
        StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_EGG_HEALER_DEAD);
        prop->model.material.diffuseMap = TEX_EGG_HEALER_DEAD;
    }
}

static void EggHealer_OnEvent(Prop* prop, Prop* sender, EventType event, int data)
{
    switch (event)
    {
        case kEventStartTurn:
        {
            if (data != prop->playerId)
                break;
            
            for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
            {
                Unit* unit = prop->engine->sceneSystem.units + i;
                if (unit->dead || unit->state == kUnitStateDead) continue;
                if (unit->playerId != prop->playerId) continue;
                
                float distSq = Vec3_DistSq(prop->position, unit->position);
                
                float effectRadius = 18.0f;
                
                if (distSq < effectRadius * effectRadius)
                {
                    if (unit->hp < unit->maxHp)
                    {
                        int hpToAdd = prop->data[0];
                        Unit_Damage(unit, NULL, hpToAdd, kDamageTypeHeal);
                        
                        PartEmitter* emitter =  PartSystem_EmitEffect(&prop->engine->partSystem, kPartEffectHeal, AABB_Center(prop->bounds), prop->rotation, 0);
                        
                        if (emitter)
                            emitter->controlPoints[0] = AABB_Center(unit->bounds);
                        
                        if (!SndSystem_IsPlaying(&prop->engine->soundSystem, SND_HEAL))                            
                            SndSystem_PlaySound(&prop->engine->soundSystem, SND_HEAL);
                    }
                }
            }
            
        }
            
        default: break;
    }
}

static void EggHealer_OnSpawn(Prop* prop, int flags)
{
    prop->onEvent = EggHealer_OnEvent;
    prop->onDamage = EggHealer_OnDamage;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_EGG_HEALER);
    prop->model.material.diffuseMap = TEX_EGG_HEALER;
    
    prop->playerId = ENGINE_PLAYER_AI;
    prop->data[0] = 20;
    prop->hp = 60;
    
    prop->touchEnabled = 1;
    
    Ray3 ray = Ray3_Create(Vec3_Offset(prop->position, 0.0f, 0.0f, 5.0f), Vec3_Create(0.0f, 0.0f, -1.0f));
    
    NavRaycastResult result;
    if (NavSystem_Raycast(&prop->engine->navSystem, ray, &result))
        prop->navPoly = result.poly;
    
    prop->bounds = AABB_CreateCentered(Vec3_Offset(prop->position, 0.0f, 0.0f, 3.0f) , Vec3_Create(3.7f, 3.7f, 5.5f));
}

static void AcidDamage_OnTouch(Prop* prop, Prop* other)
{
    Prop_Damage(other, prop, prop->hp, kDamageTypeAcid);
}

static void AcidDamage_OnTouchUnit(Prop* prop, Unit* unit)
{
    if (unit->state == kUnitStateDead)
        return;
    
    float radius = AABB_Size(prop->bounds).x / 2.0f;
    float distanceFactor = CLAMP(Vec3_Dist(unit->position, prop->position) / radius, 0.0f, 1.0f);
    
    Unit_Damage(unit, prop, prop->hp * (1.0f - distanceFactor), kDamageTypeAcid);
}

static void AcidDamage_OnTick(Prop* prop)
{
    if (--prop->timer <= 0)
    {
        if (!prop->touchEnabled)
        {
            prop->timer = 2;
            prop->touchEnabled = 1;
            prop->onTouch = AcidDamage_OnTouch;
            prop->onTouchUnit = AcidDamage_OnTouchUnit;
        }
        else
        {
            Prop_Kill(prop);
        }
    }
}

static void AcidDamage_OnSpawn(Prop* prop, int flags)
{
    if (!SndSystem_IsPlaying(&prop->engine->soundSystem, SND_MELEE))
        SndSystem_PlaySound(&prop->engine->soundSystem, SND_MELEE);
    
    prop->onTick = AcidDamage_OnTick;
    prop->bounds = AABB_CreateCentered(prop->position, Vec3_Create(25.0f, 25.0f, 10.0f));
    
    SndSystem_PlaySound(&prop->engine->soundSystem, SND_EYE_MINE_EXPLODE);
    PartSystem_EmitEffect(&prop->engine->partSystem, kPartEffectAcid, prop->position, prop->rotation, 0);

    prop->hp = 80;

    prop->visible = 0;
    prop->touchEnabled = 0;
    prop->timer = 8;
}


static void EyeMine_OnTick(Prop* prop)
{
    int isTriggered = prop->data[0];

    if (isTriggered) return;
    
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        const Unit* unit = prop->engine->sceneSystem.units + i;
        if (unit->dead) continue;
        if (unit->playerId == prop->playerId) continue;
        
        float dist = 20.0f;
        if (Vec3_DistSq(unit->position, prop->position) < dist * dist)
        {
            isTriggered = 1;
            prop->data[0] = isTriggered;
            
            SndSystem_PlaySound(&prop->engine->soundSystem, SND_EYE_MINE_OPEN);

            if (prop->timer == -1)
                prop->timer = rand() % 7 + 1;
            
            StaticModel_Shutdown(&prop->model);
            StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_EYE_MINE_OPEN);
            prop->model.material.diffuseMap = TEX_EYE_MINE;
            break;
        }
    }
    
    
}

static void EyeMine_OnEvent(Prop* prop, Prop* sender, EventType event, int data)
{
    int isTriggered = prop->data[0];
    
    switch (event)
    {
        case kEventStartTurn:
        {
            if (!isTriggered) break;
            
            --prop->timer;
            
            if (prop->timer < 1)
            {
                SceneSystem_SpawnPropAt(&prop->engine->sceneSystem, kPropAcidDamage, prop->position, prop->rotation, 0);
                Prop_Kill(prop);
            }
        }
            
        default: break;
    }
}

static void EyeMine_OnVar(Prop* prop, const char* key, const char* val)
{
    if (strcmp(key, "timer") == 0)
    {
        prop->timer = atoi(val);
    }
}

static void EyeMine_OnSpawn(Prop* prop, int flags)
{
    prop->onEvent = EyeMine_OnEvent;
    prop->onTick = EyeMine_OnTick;
    prop->onVar = EyeMine_OnVar;
    
    StaticModel_Copy(&prop->model, prop->engine->renderSystem.models + MODEL_EYE_MINE_CLOSED);
    prop->model.material.diffuseMap = TEX_EYE_MINE;
    
    prop->touchEnabled = 1;
    prop->playerId = ENGINE_PLAYER_AI;
    
    prop->timer = -1;
    
    int isTriggered = 0;
    prop->data[0] = isTriggered;
    
    prop->bounds = AABB_CreateCentered(Vec3_Offset(prop->position, 0.0f, 0.0f, 1.0f) , Vec3_Create(3.5f, 3.5f, 2.0f));
}

static const UnitSpawnTable unitSpawnTable[] = {
    kUnitScientist, Scientist_OnSpawn,
    kUnitPhantom, Phantom_OnSpawn,
    kUnitBat, Bat_OnSpawn,
    kUnitVamp, Vamp_OnSpawn,
    kUnitWolf, Wolf_OnSpawn,
    kUnitBoss, Boss_OnSpawn,
    kUnitNone, NULL,
};

static const PropSpawnTable propSpawnTable[] = {
    kPropUnitSpawn, UnitSpawn_OnSpawn,
    kPropSkull, Skull_OnSpawn,
    kPropWeapon, Weapon_OnSpawn,
    kPropMgBullet, MgBullet_OnSpawn,
    kPropRevolverBullet, RevolverBullet_OnSpawn,
    kPropCannonBullet, CannonBullet_OnSpawn,
    kPropMeleeDamage, MeleeDamage_OnSpawn,
    kPropMagicBall, MagicBall_OnSpawn,
    kPropVampBall, VampBall_OnSpawn,
    kPropMindDamage, MindDamage_OnSpawn,
    kPropWeaponPickup, WeaponPickup_OnSpawn,
    kPropItemPickup, ItemPickup_OnSpawn,
    kPropButton, Button_OnSpawn,
    kPropTrigger, Trigger_OnSpawn,
    kPropEggHealer, EggHealer_OnSpawn,
    kPropEyeMine, EyeMine_OnSpawn,
    kPropAcidDamage, AcidDamage_OnSpawn,
    kPropNone, NULL,
};

static const EnumTable propEnumTables[] = {
    "UNIT_SPAWN", kPropUnitSpawn,
    "WEAPON_PICKUP", kPropWeaponPickup,
    "ITEM_PICKUP", kPropItemPickup,
    "BUTTON", kPropButton,
    "TRIGGER", kPropTrigger,
    "EGG_HEALER", kPropEggHealer,
    "EYE_MINE", kPropEyeMine,
    "SKULL", kPropSkull,
    NULL, kPropNone
};

static const EnumTable eventEnumTables[] = {
    "SPAWN", kEventSpawn,
    "DIE", kEventDie,
    "ACTIVATE", kEventActivate,
    "DEACTIVATE", kEventDeactivate,
    "ACTION", kEventAction,
    NULL, -1,
};

const WeaponInfo g_engineWeaponTable[] = {
    /* weaponType,      name,           range,  spread, dam,    critBonus, critChance, projectile, model, texture, guiIndex */
    kWeaponStyleRanged,     "REVOLVER",     40.0f,  0.05f,  36,     15,     5, kPropRevolverBullet, {2.0f, 0.0f, 0.0f},     MODEL_REVOLVER,           TEX_REVOLVER, 8,
    kWeaponStyleRanged,     "CANNON",       55.0f,  0.1f,   60,     10,     8, kPropCannonBullet, {3.0f, 0.0f, 0.0f},       MODEL_CANNON,       TEX_CANNON, 1,
    kWeaponStyleRanged,     "MACHINE GUN",  25.0f,  0.31f,  14,     20,     3, kPropMgBullet, {3.5f, 0.0f, 0.0f},           MODEL_MG,           TEX_MG, 0,
    kWeaponStyleMelee,      "AXE",          5.0f,   0.0f,   48,     25,     4, kPropMeleeDamage, {-0.5f, 0.0f, 2.2f},       MODEL_AXE,          TEX_AXE, 2,
    kWeaponStyleMelee,      "MACHETE",      5.0f,   0.0f,   30,     20,     5, kPropMeleeDamage, {-0.5f, 0.0f, 2.2f},       MODEL_MACHETE,      TEX_MACHETE, 9,
    kWeaponStyleMelee,      "SYRINGE",      5.0f,   0.0f,   20,     30,     10, kPropMindDamage, {1.0f, 0.0f, 0.0f},         MODEL_SYRINGE,          TEX_SYRINGE, 7,
    kWeaponStyleMelee,      "PHANTOM CLAW", 5.0f,   0.0f,   55,     20,     5, kPropMeleeDamage, {0.0f, 0.0f, 0.0f},        -1, -1, 4,
    kWeaponStyleMelee,      "BAT CLAW",     5.0f,   0.0f,   12,     10,     3, kPropMeleeDamage, {0.0f, 0.0f, 0.0f},        -1, -1, 5,
    kWeaponStyleMelee,      "WOLF BITE",    5.0f,   0.0f,   40,     10,     3, kPropMeleeDamage, {0.0f, 0.0f, 0.0f},        -1, -1, 6,
    kWeaponStyleRanged,     "MAGIC BALL",   30.0f,  0.11f,  62,     10,     3, kPropMagicBall, {0.0f, 0.0f, 0.0f},          -1,   -1, 3,
    kWeaponStyleRanged,     "VAMP BALL",    35.0f,  0.16f,  32,     5,      3, kPropVampBall, {0.0f, 0.0f, 0.0f},           -1,   -1, 10,

};

const SpawnTable g_engineSpawnTable = {
    unitSpawnTable, propSpawnTable
};

const LevelLoadTable g_engineLevelLoadTable = {
    propEnumTables, eventEnumTables
};
