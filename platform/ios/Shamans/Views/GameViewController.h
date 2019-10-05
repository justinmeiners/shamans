//
//  GameViewController.h
//  Test App
//
//  Created by Justin Meiners on 6/30/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#include "engine_settings.h"
#import "unit_info.h"

@class GameViewController;

enum EngineResult;

@protocol GameViewControllerDelegate <NSObject>

- (void)gameViewControllerFinished:(GameViewController*)controller
                        withResult:(EngineResult)result
                          unitInfo:(const UnitInfo*)info
                         unitCount:(NSInteger)count;

@end

@interface GameViewController : GLKViewController

@property(nonatomic, weak)IBOutlet UILabel* loadingLabel;
@property(nonatomic, weak)NSObject<GameViewControllerDelegate>* gameDelegate;

@property (strong, nonatomic) NSString* levelPath;
@property (strong, nonatomic) NSArray* selectedUnits;


@end
