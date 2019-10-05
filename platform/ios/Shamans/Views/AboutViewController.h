//
//  AboutViewController.h
//  Shamans
//
//  Created by Justin Meiners on 12/21/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface AboutViewController : UIViewController

@property(nonatomic, weak)IBOutlet UIButton* backButton;

- (IBAction)backPressed:(id)sender;
- (IBAction)gameWebsitePressed:(id)sender;

@end
