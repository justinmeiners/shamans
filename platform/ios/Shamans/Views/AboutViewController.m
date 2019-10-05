//
//  AboutViewController.m
//  Shamans
//
//  Created by Justin Meiners on 12/21/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "AboutViewController.h"

@interface AboutViewController ()

@end

@implementation AboutViewController

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
    [self.navigationController popViewControllerAnimated:YES];
}

- (IBAction)gameWebsitePressed:(id)sender
{
    NSString* siteUrl = @"http://justinmeiners.github.io/";
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:siteUrl]];
}

@end
