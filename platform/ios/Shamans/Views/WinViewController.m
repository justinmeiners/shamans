//
//  WinViewController.m
//  Shamans
//
//  Created by Justin Meiners on 7/7/17.
//  Copyright © 2017 Justin Meiners. All rights reserved.
//

#import "WinViewController.h"

@interface WinViewController ()

@end

@implementation WinViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)backPressed:(id)sender
{
    [self.navigationController popToRootViewControllerAnimated:YES];
}

@end
