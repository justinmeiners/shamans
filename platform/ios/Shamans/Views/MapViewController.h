//
//  MapViewController.h
//  Shamans
//
//  Created by Justin Meiners on 11/18/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "CrewViewController.h"
#import "GameViewController.h"

@interface MapViewController : UIViewController <CrewViewControllerDelegate, GameViewControllerDelegate>

@property(nonatomic, weak)IBOutlet UIButton* backButton;

@property(nonatomic, weak)IBOutlet UIButton* storage1Button;
@property(nonatomic, weak)IBOutlet UIButton* storage2Button;
@property(nonatomic, weak)IBOutlet UIButton* storage3Button;
@property(nonatomic, weak)IBOutlet UIButton* storage4Button;
@property(nonatomic, weak)IBOutlet UIButton* storageShamanButton;

@property(nonatomic, weak)IBOutlet UIButton* lab1Button;
@property(nonatomic, weak)IBOutlet UIButton* lab2Button;
@property(nonatomic, weak)IBOutlet UIButton* lab3Button;

@property(nonatomic, weak)IBOutlet UIButton* plant1Button;
@property(nonatomic, weak)IBOutlet UIButton* plant2Button;
@property(nonatomic, weak)IBOutlet UIButton* plantShamanButton;

@property(nonatomic, weak)IBOutlet UIButton* observatory1Button;
@property(nonatomic, weak)IBOutlet UIButton* observatory2Button;
@property(nonatomic, weak)IBOutlet UIButton* observatoryShamanButton;

- (IBAction)levelButtonPressed:(id)sender;
- (IBAction)backButtonPressed:(id)sender;

@end
