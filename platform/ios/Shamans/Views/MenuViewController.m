//
//  MenuViewController.m
//  Shamans
//
//  Created by Justin Meiners on 11/18/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "MenuViewController.h"
#import "DataManager.h"

@interface MenuViewController ()

@end

@implementation MenuViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [[DataManager sharedManager] loadProfile];
    
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)playPressed:(id)sender
{
    [self performSegueWithIdentifier:@"map" sender:NULL];
}

- (IBAction)aboutPressed:(id)sender
{
    [self performSegueWithIdentifier:@"about" sender:NULL];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
