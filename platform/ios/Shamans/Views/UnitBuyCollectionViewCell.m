//
//  UnitBuyCollectionViewCell.m
//  Shamans
//
//  Created by Justin Meiners on 1/2/17.
//  Copyright © 2017 Justin Meiners. All rights reserved.
//

#import "UnitBuyCollectionViewCell.h"

@implementation UnitBuyCollectionViewCell

- (void)awakeFromNib
{
    [super awakeFromNib];
    
    self.selectedBackgroundView.layer.borderColor = [[UIColor colorWithWhite:0.7f alpha:1.0f] CGColor];
    self.selectedBackgroundView.layer.borderWidth = 1.0f;
    self.selectedBackgroundView.layer.cornerRadius = 8.0f;
}

@end
