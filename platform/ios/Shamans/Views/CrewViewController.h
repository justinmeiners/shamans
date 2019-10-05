//
//  CrewViewController.h
//  Shamans
//
//  Created by Justin Meiners on 12/29/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import <UIKit/UIKit.h>

@class CrewViewController;

@protocol CrewViewControllerDelegate <NSObject>

- (void)crewViewController:(CrewViewController*)vc selectedUnits:(NSArray*)units;
- (void)crewViewControllerCancelled:(CrewViewController*)vc;

@end

@interface CrewViewController : UIViewController <UICollectionViewDataSource, UICollectionViewDelegate, UIGestureRecognizerDelegate>

@property(nonatomic, weak)IBOutlet UICollectionView* unitCollectionView;
@property(nonatomic, weak)IBOutlet UIButton* playButton;
@property(nonatomic, weak)IBOutlet UIImageView* levelImageView;
@property(nonatomic, weak)IBOutlet UILabel* levelTitleLabel;
@property(nonatomic, weak)IBOutlet UILabel* selectionProgressLabel;

@property(nonatomic, weak)IBOutlet UILabel* nameLabel;
@property(nonatomic, weak)IBOutlet UILabel* typeLabel;
@property(nonatomic, weak)IBOutlet UILabel* battlesLabel;
@property(nonatomic, weak)IBOutlet UILabel* primaryLabel;
@property(nonatomic, weak)IBOutlet UILabel* secondaryLabel;
@property(nonatomic, weak)IBOutlet UILabel* attemptsLabel;

@property(nonatomic, weak)IBOutlet UIButton* changeNameButton;

@property(nonatomic, weak)id <CrewViewControllerDelegate> delegate;

@property (strong, nonatomic) NSDictionary* levelInfo;

@property(nonatomic, assign)NSInteger maxUnitCount;
@property(nonatomic, assign)NSInteger selectedUnitCount;

- (IBAction)playPressed:(id)sender;
- (IBAction)editNamePressed:(id)sender;
- (IBAction)giveUpPressed:(id)sender;

@end
