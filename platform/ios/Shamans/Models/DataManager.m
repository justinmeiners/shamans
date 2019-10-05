//
//  DataManager.m
//  Shamans
//
//  Created by Justin Meiners on 12/30/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "DataManager.h"

static NSString * const PROFILE_FILENAME = @"profile.json";
static NSString * const CREW_FILENAME = @"crew.json";

static NSString * const VISIBLE_LEVELS_KEY = @"visible_levels";
static NSString * const COMPLETED_LEVELS_KEY = @"completed_levels";
static NSString * const UNLOCK_LEVEL_KEY = @"unlock_level";
static NSString * const KEY_KEY = @"key";
static NSString * const ATTEMPTS_KEY = @"attempts";

@implementation DataManager

+ (NSURL *)applicationDocumentsDirectory
{
    return [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
}

- (id)init
{
    self = [super init];
    
    if (self)
    {        
        self.crewUnits = [NSMutableArray array];
        self.completedLevels = [NSMutableArray array];
        self.visibleLevels = [NSMutableArray array];
        
        self.levelInfo = [NSMutableDictionary dictionaryWithContentsOfURL:[[NSBundle mainBundle] URLForResource:@"Maps" withExtension:@"plist"]];
        
        self.attempts = 1;
        
        // add keys to dictionary so levels can self-identify
        for (NSString* key in _levelInfo)
        {
            [[_levelInfo objectForKey:key] setObject:key forKey:KEY_KEY];
        }
    }
    
    return self;
}

+ (id)sharedManager
{
    static DataManager* sharedManager = NULL;
    
    static dispatch_once_t onceToken;
    
    dispatch_once(&onceToken, ^{
        sharedManager = [[self alloc] init];
    });
    
    return sharedManager;
}

- (UnitInfo*)allocUnitTransaction:(NSArray*)array
{
    size_t count = [array count];
    
    UnitInfo* buffer = calloc(count, sizeof(UnitInfo));
    
    for (int i = 0; i < count; ++i)
    {
        UnitInfo* toWrite = buffer + i;
        UnitObject* object = [array objectAtIndex:i];
        
        toWrite->type = [object type];
        toWrite->primaryWeapon = [object primaryWeaponType];
        toWrite->secondaryWeapon = [object secondaryWeaponType];
        toWrite->crewIndex = (int)[_crewUnits indexOfObject:object];
        
        const char* string = [[object name] cStringUsingEncoding:NSUTF8StringEncoding];
        UnitInfo_SetName(toWrite, string);
    }
    
    return buffer;
}

- (TransactionReport)completeUnitTransaction:(const UnitInfo*)units count:(NSInteger)count
{
    NSMutableArray* toDelete = [NSMutableArray array];
    NSMutableArray* toAdd = [NSMutableArray array];
    
    TransactionReport report;
    report.acquired = 0;
    report.killed = 0;
    report.updated = 0;
    
    for (int i = 0; i < count; ++i)
    {
        const UnitInfo* unitInfo = units + i;
        
        if (unitInfo->type == kUnitNone)
        {
            // Handle deletions
            [toDelete addObject:[_crewUnits objectAtIndex:unitInfo->crewIndex]];
            ++report.killed;
        }
        else if (unitInfo->crewIndex == -1)
        {
            // Handle acquired units
            UnitObject* newUnit = [[UnitObject alloc] initWithUnitInfo:unitInfo];
            [newUnit setBattleCount:1];
            [toAdd addObject:newUnit];
            ++report.acquired;
        }
        else
        {
            // Persistent/Modified units
            UnitObject* unit = [_crewUnits objectAtIndex:unitInfo->crewIndex];
            [unit setBattleCount:unit.battleCount + 1];
            [unit setType:(int)unitInfo->type];
            [unit setPrimaryWeaponType:(int)unitInfo->primaryWeapon];
            [unit setSecondaryWeaponType:(int)unitInfo->secondaryWeapon];
            ++report.updated;
        }
    }
    
    [_crewUnits removeObjectsInArray:toDelete];
    [_crewUnits addObjectsFromArray:toAdd];
    
    return report;
}

- (void)loadProfile
{
    NSURL* documentsRootUrl = [DataManager applicationDocumentsDirectory];

    NSError* error = NULL;
    NSURL* profileUrl = [documentsRootUrl URLByAppendingPathComponent:PROFILE_FILENAME];
    NSData* profileJson = [NSData dataWithContentsOfURL:profileUrl];
    
    if (profileJson)
    {
        NSDictionary* profile = [NSJSONSerialization JSONObjectWithData:profileJson options:0 error:&error];
        
        if (error != NULL)
        {
            NSLog(@"%@", error);
        }
        else
        {
            NSArray* completedLevels = [profile objectForKey:COMPLETED_LEVELS_KEY];
            [_completedLevels removeAllObjects];
            [_completedLevels addObjectsFromArray:completedLevels];
            
            NSArray* visibleLevels = [profile objectForKey:VISIBLE_LEVELS_KEY];
            [_visibleLevels removeAllObjects];
            [_visibleLevels addObjectsFromArray:visibleLevels];
            
            NSNumber* attempts = [profile objectForKey:ATTEMPTS_KEY];
            self.attempts = [attempts integerValue];
        }
    }
    
    error = NULL;
    NSURL* crewUnitsUrl = [documentsRootUrl URLByAppendingPathComponent:CREW_FILENAME];
    NSData* crewUnitsJson = [NSData dataWithContentsOfURL:crewUnitsUrl];
    
    if (crewUnitsJson)
    {
        NSArray* unitJsons = [NSJSONSerialization JSONObjectWithData:crewUnitsJson options:0 error:&error];

        if (error != NULL)
        {
            NSLog(@"%@", error);
        }
        else
        {
            [_crewUnits removeAllObjects];
            
            for (NSDictionary* unitJson in unitJsons)
            {
                [_crewUnits addObject:[[UnitObject alloc] initWithJson:unitJson]];
            }
        }
    }
    
    if ([_visibleLevels count] < 1)
    {
        [_visibleLevels addObject:@"storage_1"];

        [_visibleLevels addObject:@"storage_2"];
        [_visibleLevels addObject:@"storage_3"];
        [_visibleLevels addObject:@"storage_4"];
        [_visibleLevels addObject:@"storage_shaman"];
        [_visibleLevels addObject:@"lab_1"];
        [_visibleLevels addObject:@"lab_2"];
        [_visibleLevels addObject:@"lab_3"];
        [_visibleLevels addObject:@"plant_1"];
        [_visibleLevels addObject:@"plant_2"];
        [_visibleLevels addObject:@"plant_shaman"];
        [_visibleLevels addObject:@"observatory_1"];
        [_visibleLevels addObject:@"observatory_2"];
        [_visibleLevels addObject:@"observatory_shaman"];
        
        [self resetCrew];

    }
}

- (void)saveProfile
{
    NSURL* documentsRootUrl = [DataManager applicationDocumentsDirectory];
    
    NSMutableDictionary* profileData = [NSMutableDictionary dictionary];
    
    [profileData setObject:_completedLevels forKey:COMPLETED_LEVELS_KEY];
    [profileData setObject:_visibleLevels forKey:VISIBLE_LEVELS_KEY];
    [profileData setObject:[NSNumber numberWithInteger:_attempts] forKey:ATTEMPTS_KEY];

    NSError* error = NULL;
    NSURL* profileUrl = [documentsRootUrl URLByAppendingPathComponent:PROFILE_FILENAME];
    NSData* profileJson = [NSJSONSerialization dataWithJSONObject:profileData options:NSJSONWritingPrettyPrinted error:&error];
    [profileJson writeToURL:profileUrl atomically:YES];
    
    if (error != NULL)
    {
        NSLog(@"%@", error);
    }
    
    error = NULL;
    NSURL* crewUnitsUrl = [documentsRootUrl URLByAppendingPathComponent:CREW_FILENAME];
    NSData* crewUnitsJson = [NSJSONSerialization dataWithJSONObject:[UnitObject arrayToJson:_crewUnits] options:NSJSONWritingPrettyPrinted error:&error];
    [crewUnitsJson writeToURL:crewUnitsUrl atomically:YES];
    
    if (error != NULL)
    {
        NSLog(@"%@", error);
    }
}

- (NSArray*)completeLevel:(NSDictionary*)levelInfo
{
    if (![_completedLevels containsObject:[levelInfo objectForKey:KEY_KEY]])
    {
        NSArray* unlockedLevels = [levelInfo objectForKey:UNLOCK_LEVEL_KEY];
        [_visibleLevels addObjectsFromArray:unlockedLevels];
        [_completedLevels addObject:[levelInfo objectForKey:KEY_KEY]];
        return unlockedLevels;
    }

    return @[];
}

- (void)resetCrew
{
    [_crewUnits removeAllObjects];
    
    
    const NSInteger defaultUnitCount = 12;
    
    for (NSInteger i = 0; i < defaultUnitCount; ++i)
    {
        UnitObject* object = [[UnitObject alloc] init];
        [object setName:[NSString stringWithFormat:@"Scientist %lu", (long)i]];
        [object setType:kUnitScientist];
        [object setPrimaryWeaponType:kWeaponRevolver];
        [object setSecondaryWeaponType:kWeaponAxe];
        [_crewUnits addObject:object];
    }
    
/*
    for (NSInteger i = 0; i < 3; ++i)
    {
        UnitObject* object = [[UnitObject alloc] init];
        [object setName:[NSString stringWithFormat:@"Scientist %lu", (long)i]];
        [object setType:kUnitScientist];
        [object setPrimaryWeaponType:kWeaponMg];
        [object setSecondaryWeaponType:kWeaponAxe];
        [_crewUnits addObject:object];
    }
    
    for (NSInteger i = 0; i < 3; ++i)
    {
        UnitObject* object = [[UnitObject alloc] init];
        [object setName:[NSString stringWithFormat:@"Scientist %lu", (long)i]];
        [object setType:kUnitScientist];
        [object setPrimaryWeaponType:kWeaponRevolver];
        [object setSecondaryWeaponType:kWeaponMachete];
        [_crewUnits addObject:object];
    }
    
    for (NSInteger i = 0; i < 3; ++i)
    {
        UnitObject* object = [[UnitObject alloc] init];
        [object setName:[NSString stringWithFormat:@"Scientist %lu", (long)i]];
        [object setType:kUnitScientist];
        [object setPrimaryWeaponType:kWeaponCannon];
        [object setSecondaryWeaponType:kWeaponMachete];
        [_crewUnits addObject:object];
    }
    
    for (NSInteger i = 0; i < 3; ++i)
    {
        UnitObject* object = [[UnitObject alloc] init];
        [object setName:[NSString stringWithFormat:@"Scientist %lu", (long)i]];
        [object setType:kUnitScientist];
        [object setPrimaryWeaponType:kWeaponSyringe];
        [object setSecondaryWeaponType:kWeaponAxe];
        [_crewUnits addObject:object];
    }
 */
}

@end
