//
//  MenuViewController.h
//  Shamans
//
//  Created by Justin Meiners on 11/18/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MenuViewController : UIViewController

@property(nonatomic, weak)IBOutlet UIButton* playButton;
@property(nonatomic, weak)IBOutlet UIButton* aboutButton;
@property(nonatomic, weak)IBOutlet UIStoryboardSegue* mapSegue;

- (IBAction)playPressed:(id)sender;
- (IBAction)aboutPressed:(id)sender;

@end
