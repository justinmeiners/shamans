//
//  UnitObject.h
//  Shamans
//
//  Created by Justin Meiners on 12/29/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "unit_info.h"

@interface UnitObject : NSObject <NSCopying>

@property(nonatomic, copy)NSString* name;
@property(nonatomic, assign)UnitType type;
@property(nonatomic, assign)NSInteger battleCount;
@property(nonatomic, assign)WeaponType primaryWeaponType;
@property(nonatomic, assign)WeaponType secondaryWeaponType;

- (id)initWithUnitInfo:(const UnitInfo*)unitInfo;

- (id)initWithJson:(NSDictionary*)json;
- (NSDictionary*)toJson;

+ (NSArray*)arrayToJson:(NSArray*)unitObjects;

+ (NSString*)typeToString:(UnitType)type;
+ (NSString*)weaponToString:(WeaponType)type;

@end
