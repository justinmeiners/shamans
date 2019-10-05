//
//  UnitObject.m
//  Shamans
//
//  Created by Justin Meiners on 12/29/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "UnitObject.h"

static NSString * const NAME_KEY = @"name";
static NSString * const TYPE_KEY = @"type";
static NSString * const BATTLE_COUNT_KEY = @"battleCount";

static NSString * const PRIMARY_WEAPON_KEY = @"primaryWeapon";
static NSString * const SECONDARY_WEAPON_KEY = @"secondaryWeapon";


@interface UnitObject ()
@property(nonatomic, copy)NSString* unitName;
@end

@implementation UnitObject

- (id)initWithUnitInfo:(const UnitInfo*)unitInfo
{
    self = [super init];
    
    if (self)
    {
        self.name = [NSString stringWithUTF8String:unitInfo->name];
        _type = (int)unitInfo->type;
        
        
        _battleCount = 0;
        _primaryWeaponType = unitInfo->primaryWeapon;
        _secondaryWeaponType = unitInfo->secondaryWeapon;
    }
    
    return self;
}

- (id)initWithJson:(NSDictionary*)json
{
    self = [super init];
    
    if (self)
    {
        self.name = [json objectForKey:NAME_KEY];
        _type = (UnitType)[[json objectForKey:TYPE_KEY] integerValue];
        _battleCount = [[json objectForKey:BATTLE_COUNT_KEY] integerValue];

        _primaryWeaponType = (WeaponType)[[json objectForKey:PRIMARY_WEAPON_KEY] integerValue];
        _secondaryWeaponType = (WeaponType)[[json objectForKey:SECONDARY_WEAPON_KEY] integerValue];
    }
    
    return self;
}

- (NSDictionary*)toJson
{
    NSDictionary* json = @{
                        NAME_KEY: _name,
                        TYPE_KEY: [NSNumber numberWithInteger: (NSInteger)_type],
                        BATTLE_COUNT_KEY: [NSNumber numberWithInteger:_battleCount],
                        PRIMARY_WEAPON_KEY: [NSNumber numberWithInteger: (NSInteger)_primaryWeaponType],
                        SECONDARY_WEAPON_KEY: [NSNumber numberWithInteger:(NSInteger)_secondaryWeaponType]
                           };
    
    return json;
}

+ (NSArray*)arrayToJson:(NSArray*)unitObjects
{
    NSMutableArray* toReturn = [NSMutableArray array];
    
    for (UnitObject* unit in unitObjects)
    {
        [toReturn addObject:[unit toJson]];
    }
    
    return toReturn;
}

+ (NSString*)typeToString:(UnitType)type
{
    switch (type)
    {
        case kUnitBat:
            return @"Bat";
        case kUnitBoss:
            return @"Witch";
        case kUnitVamp:
            return @"Vampire";
        case kUnitPhantom:
            return @"Phantom";
        case kUnitScientist:
            return @"Scientist";
        case kUnitWolf:
            return @"Wolf";
        default:
            return NULL;
    };
}

+ (NSString*)weaponToString:(WeaponType)type
{
    switch (type)
    {
        case kWeaponMg:
            return @"Mg";
        case kWeaponAxe:
            return @"Axe";
        case kWeaponCannon:
            return @"Cannon";
        case kWeaponRevolver:
            return @"Revolver";
        case kWeaponMachete:
            return @"Machete";
        case kWeaponSyringe:
            return @"Syring";
        default:
            return NULL;
    };
}

- (id)copyWithZone:(NSZone *)zone
{
    id copy = [[[self class] allocWithZone:zone] init];
    
    if (copy)
    {
        [copy setType:_type];
        [copy setName:_name];
        [copy setBattleCount:_battleCount];
        [copy setPrimaryWeaponType:_primaryWeaponType];
        [copy setSecondaryWeaponType:_secondaryWeaponType];
    }
    
    return copy;
}

@end
