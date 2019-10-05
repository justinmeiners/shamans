//
//  UnitCollectionViewCell.m
//  Shamans
//
//  Created by Justin Meiners on 12/29/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "UnitCollectionViewCell.h"

@implementation UnitCollectionViewCell


- (void)awakeFromNib
{
    [super awakeFromNib];
    
    self.selectedBackgroundView.layer.borderColor = [[UIColor colorWithWhite:0.7f alpha:1.0f] CGColor];
    self.selectedBackgroundView.layer.borderWidth = 1.0f;
    self.selectedBackgroundView.layer.cornerRadius = 8.0f;
}

@end
