//
//  MapViewController.m
//  Shamans
//
//  Created by Justin Meiners on 11/18/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "MapViewController.h"
#import "GameViewController.h"
#import "DataManager.h"

static NSString * const PATH_KEY = @"path";
static NSString * const FINAL_KEY = @"final";


@interface MapViewController  ()
@property(nonatomic, strong)NSArray* levelButtons;
@property(nonatomic, strong)NSArray* levelButtonKeys;

@property(nonatomic, copy)NSString* mapToLoadKey;
@property(nonatomic, strong)NSArray* selectedUnits;
@end

@implementation MapViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.levelButtons = @[_storage1Button,
                          _storage2Button,
                          _storage3Button,
                          _storage4Button,
                          _storageShamanButton,
                          _lab1Button,
                          _lab2Button,
                          _lab3Button,
                          _plant1Button,
                          _plant2Button,
                          _plantShamanButton,
                          _observatory1Button,
                          _observatory2Button,
                          _observatoryShamanButton];
    
    self.levelButtonKeys = @[@"storage_1",
                             @"storage_2",
                             @"storage_3",
                             @"storage_4",
                             @"storage_shaman",
                             @"lab_1",
                             @"lab_2",
                             @"lab_3",
                             @"plant_1",
                             @"plant_2",
                             @"plant_shaman",
                             @"observatory_1",
                             @"observatory_2",
                             @"observatory_shaman"];
    
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewDidLoad];
    
    NSArray* visibleLevels = [[DataManager sharedManager] visibleLevels];
    NSArray* completedLevels = [[DataManager sharedManager] completedLevels];
    
    for (NSUInteger i = 0; i < [_levelButtons count]; ++i)
    {
        BOOL visible = [visibleLevels containsObject: [_levelButtonKeys objectAtIndex:i]];
        [[_levelButtons objectAtIndex:i] setHidden:!visible];
        
        BOOL completed = [completedLevels containsObject: [_levelButtonKeys objectAtIndex:i]];
        [[_levelButtons objectAtIndex:i] setSelected:!completed];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (IBAction)levelButtonPressed:(id)sender
{
    for (NSUInteger i = 0; i < [_levelButtons count]; ++i)
    {
        if ([_levelButtons objectAtIndex:i] == sender)
        {
            self.mapToLoadKey = [_levelButtonKeys objectAtIndex:i];
            break;
        }
    }

    [self performSegueWithIdentifier:@"crew" sender:NULL];
}


- (IBAction)backButtonPressed:(id)sender
{
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)crewViewController:(CrewViewController*)vc selectedUnits:(NSArray*)units
{
    self.selectedUnits = units;
    [[DataManager sharedManager] saveProfile];
    
    [self dismissViewControllerAnimated:NO completion:^{
        [self performSegueWithIdentifier:@"game" sender:NULL];
    }];
}

- (void)crewViewControllerCancelled:(CrewViewController*)vc
{
    [self dismissViewControllerAnimated:YES completion:NULL];
    [[DataManager sharedManager] saveProfile];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    NSDictionary* levelInfo = [[DataManager sharedManager] levelInfo];
    
    if ([segue.identifier isEqualToString:@"game"])
    {
        GameViewController* gameVc = [segue destinationViewController];
        NSDictionary* mapEntry = [levelInfo objectForKey:_mapToLoadKey];
        gameVc.gameDelegate = self;
        gameVc.levelPath = [mapEntry objectForKey:PATH_KEY];
        gameVc.selectedUnits = _selectedUnits;
    }
    else if ([segue.identifier isEqualToString:@"crew"])
    {
        CrewViewController* crewVc = [segue destinationViewController];
        NSDictionary* mapEntry = [levelInfo objectForKey:_mapToLoadKey];
        crewVc.levelInfo = mapEntry;
        crewVc.delegate = self;
    }
}

- (void)gameViewControllerFinished:(GameViewController*)controller
                        withResult:(EngineResult)result
                          unitInfo:(const UnitInfo*)unitInfo
                         unitCount:(NSInteger)unitCount
{
    
    NSDictionary* levelInfo = [[DataManager sharedManager] levelInfo];
    NSDictionary* mapEntry = [levelInfo objectForKey:_mapToLoadKey];
    
    if (result == kEngineResultVictory)
    {
        TransactionReport report = [[DataManager sharedManager] completeUnitTransaction:unitInfo count:unitCount];
        NSArray* unlockedLevels = [[DataManager sharedManager] completeLevel:mapEntry];
        [[DataManager sharedManager] saveProfile];
        
        BOOL isFinalLevel = NO;
        
        NSNumber* value = NULL;
        if ((value = [mapEntry objectForKey:FINAL_KEY]))
        {
            isFinalLevel = [value boolValue];
        }
        
        [CATransaction begin];
        [CATransaction setCompletionBlock:^{
            if (isFinalLevel)
            {
                [self performSegueWithIdentifier:@"win" sender:NULL];
            }
            else
            {
                NSMutableString* message = [NSMutableString stringWithFormat:@"Excellent work! %li crew members were killed.", (long)report.killed];
                
                if (report.acquired > 0)
                {
                    [message appendFormat:@" %li new members were capture.", (long)report.acquired];
                }
                
                [message appendFormat:@" %li survived the battle", (long)report.updated];
                
                if ([unlockedLevels count] > 0)
                {
                    [message appendString:@" You unlocked"];
                    
                    for (NSString* key in unlockedLevels)
                    {
                        NSDictionary* unlocked = [[[DataManager sharedManager] levelInfo] objectForKey:key];
                        [message appendFormat:@" %@", [unlocked objectForKey:@"title"]];
                    }
                }
        
                UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Victory!"
                                                                               message:message
                                                                        preferredStyle:UIAlertControllerStyleAlert];
                
                [alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                    
                    
                }]];
                [self presentViewController:alert animated:YES completion:nil];
            }
        }];
        
        [self.navigationController popViewControllerAnimated:YES];
        [CATransaction commit];
    }
    else
    {
        [self.navigationController popViewControllerAnimated:YES];
    }
    
}


@end
