//
//  DataManager.h
//  Shamans
//
//  Created by Justin Meiners on 12/30/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "UnitObject.h"

typedef struct
{
    NSInteger killed;
    NSInteger acquired;
    NSInteger updated;
} TransactionReport;

@interface DataManager : NSObject

@property(nonatomic, strong)NSMutableArray* crewUnits;
@property(nonatomic, strong)NSMutableArray* completedLevels;
@property(nonatomic, strong)NSMutableArray* visibleLevels;

@property(nonatomic, strong)NSDictionary* levelInfo;

@property(nonatomic, assign)NSInteger attempts;

+ (id)sharedManager;
+ (NSURL *)applicationDocumentsDirectory;

- (UnitInfo*)allocUnitTransaction:(NSArray*)selection;
- (TransactionReport)completeUnitTransaction:(const UnitInfo*)units count:(NSInteger)count;

- (void)loadProfile;
- (void)saveProfile;

- (NSArray*)completeLevel:(NSDictionary*)levelInfo;

- (void)resetCrew;

@end
