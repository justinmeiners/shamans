
#ifndef UNIT_INFO_H
#define UNIT_INFO_H

#include "vec_math.h"
#include <stdlib.h>

typedef enum
{
    kUnitNone = -1,
    kUnitScientist = 0,
    kUnitPhantom,
    kUnitBat,
    kUnitVamp,
    kUnitWolf,
    kUnitBoss
} UnitType;


#define UNIT_NAME_MAX 32
#define UNIT_HP_MAX 180.0f
#define UNIT_RANGE_MAX 100.0f
#define UNIT_ANGLE_SPEED 5.0f


#define WEAPON_NAME_MAX 32

typedef enum
{
    kWeaponNone = -1,
    kWeaponRevolver,
    kWeaponCannon,
    kWeaponMg,
    kWeaponAxe,
    kWeaponMachete,
    kWeaponSyringe,
    kWeaponPhantomClaw,
    kWeaponBatClaw,
    kWeaponWolfBite,
    kWeaponMagicBall,
    kWeaponVampBall,
    kWeaponSpawner,
} WeaponType;

typedef enum
{
    kUnitPowerupNone = 0,
    kUnitPowerupRange = 1 << 0,
    kUnitPowerupDefense = 1 << 1,
} UnitPowerup;

typedef struct
{
    int crewIndex;
    char name[UNIT_NAME_MAX];
    UnitType type;
    WeaponType primaryWeapon;
    WeaponType secondaryWeapon;
} UnitInfo;

extern void UnitInfo_SetName(UnitInfo* info, const char* name);


typedef enum
{
    kWeaponStyleNone = 0,
    kWeaponStyleMelee,
    kWeaponStyleRanged,
} WeaponStyle;

typedef struct
{
    WeaponStyle style;
    char name[WEAPON_NAME_MAX];
    
    float range;
    float spread;
    int damage;
    
    int critBonus;
    int critChance;
    
    int projectilePropType;
    Vec3 projectileOffset;
    
    int model;
    int texture;
    int guiIndex;
    
} WeaponInfo;

static inline Vec3 WeaponInfo_CalcRandSpread(float spread, Vec3 direction)
{
    Vec3 up = Vec3_Create(0.0f, 0.0f, 1.0f);
    Vec3 right = Vec3_Cross(up, direction);
    
    Vec2 randSample = Vec2_Create(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
    
    // ranges from [-0.5f, 0.5f]
    Vec2 offset = Vec2_Create(randSample.x - 0.5f, randSample.y - 0.5f);
    
    Vec3 offsetVec = Vec3_Add(Vec3_Scale(right, offset.x * spread * 2.0f), Vec3_Scale(up, offset.y * spread));
        
    return Vec3_Norm(Vec3_Add(direction, offsetVec));
}

/*
 #define BUFFER_MAX 1024
 
 void UnitInfo_WriteToFile(const UnitInfo* unitInfos, const int unitCount, FILE* outFile)
 {
 char lineBuffer[BUFFER_MAX];
 
 sprintf(lineBuffer, "unit_count: %i\n", unitCount);
 fwrite(lineBuffer, 1, strlen(lineBuffer), outFile);
 
 int i;
 for (i = 0; i < unitCount; ++i)
 {
 const UnitInfo* unitInfo = unitInfos + i;
 
 sprintf(lineBuffer, "unit: %s, %i, %i, %i\n", unitInfo->name, unitInfo->type, unitInfo->primaryWeapon, unitInfo->secondaryWeapon);
 fwrite(lineBuffer, 1, strlen(lineBuffer), outFile);
 }
 }
 */


#endif
