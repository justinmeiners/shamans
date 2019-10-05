//
//  CrewViewController.m
//  Shamans
//
//  Created by Justin Meiners on 12/29/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "CrewViewController.h"
#import "DataManager.h"
#import "UnitBuyCollectionViewCell.h"
#import "UnitCollectionViewCell.h"
#import "UnitCollectionReusableView.h"

@interface CrewViewController ()
@property(nonatomic, strong)NSMutableArray* crewUnits;
@property(nonatomic, strong)NSMutableArray* selection;
@property(nonatomic, strong)UnitObject* currentUnit;

@property(nonatomic, strong)UITapGestureRecognizer* tapOutsideModalGesture;
@property(nonatomic, strong)UILongPressGestureRecognizer* longPressGesture;
@end

@implementation CrewViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    self.longPressGesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longPress:)];
    [_unitCollectionView addGestureRecognizer:_longPressGesture];
    
    _unitCollectionView.allowsSelection = YES;
    _unitCollectionView.allowsMultipleSelection = YES;
    
    self.crewUnits = [[DataManager sharedManager] crewUnits];
    self.selection = [NSMutableArray array];
    
    _maxUnitCount = 0;
    _selectedUnitCount = 0;
    
    [self updateRecentUnit:NULL];
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    _attemptsLabel.text = [NSString stringWithFormat:@"Continues: %lu", (long)[[DataManager sharedManager] attempts]];

}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)playPressed:(id)sender
{
    [_delegate crewViewController:self selectedUnits:_selection];
}

- (IBAction)editNamePressed:(id)sender
{
    if (!_currentUnit)
        return;
    
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Change Name"
                                                                   message:@""
                                                            preferredStyle:UIAlertControllerStyleAlert];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        _currentUnit.name = alert.textFields[0].text;
        [self updateRecentUnit:_currentUnit];

    }]];
    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.placeholder = @"Enter name:";
        textField.text = _currentUnit.name;
    }];
    [self presentViewController:alert animated:YES completion:nil];
}

- (IBAction)giveUpPressed:(id)sender
{
    NSString* message = @"All of your crew members will be deleted and replaced. Your level progress will remain unchanged.";
    
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Give Up"
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"Give Up" style:UIAlertActionStyleDestructive handler:^(UIAlertAction * _Nonnull action) {
        DataManager* dm = [DataManager sharedManager];
        [dm resetCrew];
        [dm saveProfile];
        
        dm.attempts += 1;
        
        [_unitCollectionView reloadData];
        [self.view setNeedsLayout];

    }]];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        
    }]];
    
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    NSString* thumbName = [_levelInfo objectForKey:@"thumb"];
    
    if (thumbName)
    {
        _levelImageView.image = [UIImage imageNamed:thumbName];
    }
    
    _levelTitleLabel.text = [_levelInfo objectForKey:@"title"];
    
    _maxUnitCount = [[_levelInfo objectForKey:@"unit_count"] intValue];
    _selectedUnitCount = 0;
    
    [self updateSelection];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    if (!_tapOutsideModalGesture)
    {
        self.tapOutsideModalGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapBehind:)];
        _tapOutsideModalGesture.numberOfTapsRequired = 1;
        _tapOutsideModalGesture.cancelsTouchesInView = NO;
        _tapOutsideModalGesture.delegate = self;
        [self.view.window addGestureRecognizer:_tapOutsideModalGesture];
    }
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];

    [self.view.window removeGestureRecognizer:_tapOutsideModalGesture];
    self.tapOutsideModalGesture = NULL;
}

- (void)tapBehind:(UITapGestureRecognizer *)sender
{
    if (sender.state == UIGestureRecognizerStateEnded)
    {
        CGPoint location = [sender locationInView:self.view];
        
        if (![self.view pointInside:location withEvent:nil])
        {
            [_delegate crewViewControllerCancelled:self];
        }
    }
}

- (void)longPress:(UILongPressGestureRecognizer *)gesture
{
    CGPoint gesturePoint = [gesture locationInView:_unitCollectionView];
    NSIndexPath* indexPath;
    
    switch (gesture.state)
    {
        case UIGestureRecognizerStateBegan:
            indexPath = [_unitCollectionView indexPathForItemAtPoint:gesturePoint];
            
            if (indexPath)
            {
                [_unitCollectionView beginInteractiveMovementForItemAtIndexPath:indexPath];
            }
            
            break;
        case UIGestureRecognizerStateChanged:
            [_unitCollectionView updateInteractiveMovementTargetPosition:gesturePoint];
            break;
        case UIGestureRecognizerStateEnded:
            [_unitCollectionView endInteractiveMovement];
            break;
        case UIGestureRecognizerStateCancelled:
            [_unitCollectionView cancelInteractiveMovement];
            break;
        default: break;
    }
}


- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return [_crewUnits count];
}

- (UICollectionReusableView*)collectionView:(UICollectionView *)collectionView
          viewForSupplementaryElementOfKind:(NSString *)kind
                                atIndexPath:(NSIndexPath *)indexPath
{
    if ([kind isEqualToString:UICollectionElementKindSectionHeader])
    {
        UnitCollectionReusableView* view = [collectionView dequeueReusableSupplementaryViewOfKind:kind withReuseIdentifier:@"section_header" forIndexPath:indexPath];
        
        if (indexPath.section == 0)
        {
            view.label.text = @"Select Units";
        }

        return view;
    }
    else
    {
        UICollectionReusableView* view = [collectionView dequeueReusableSupplementaryViewOfKind:kind withReuseIdentifier:@"footer" forIndexPath:indexPath];

        return view;
    }
}



- (UIImage*)tileForUnitType:(int)unitType weapon:(int)primaryWeapon
{
    NSString* imageName = NULL;
    
    switch (unitType)
    {
        case kUnitScientist:
        {
            switch (primaryWeapon)
            {
                case kWeaponMg:
                    imageName = @"scientist_mg.png";
                    break;
                case kWeaponRevolver:
                    imageName = @"scientist_revolver.png";
                    break;
                case kWeaponCannon:
                    imageName = @"scientist_cannon.png";
                    break;
                case kWeaponSyringe:
                    imageName = @"scientist_syringe.png";
                    break;
            }
            break;
        }
        case kUnitBat:
            imageName = @"bat.png";
            break;
        case kUnitVamp:
            imageName = @"vamp.png";
            break;
        case kUnitPhantom:
            imageName = @"phantom.png";
            break;
        case kUnitWolf:
            imageName = @"wolf.png";
            break;
    }
    
    if (imageName != NULL)
    {
        return [UIImage imageNamed:imageName];
    }
    else
    {
        return NULL;
    }
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    UnitCollectionViewCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"unit" forIndexPath:indexPath];
    UnitObject* unit  = [_crewUnits objectAtIndex:indexPath.item];
    
    cell.imageView.image = [self tileForUnitType:[unit type] weapon:[unit primaryWeaponType]];
    
    return cell;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (BOOL)collectionView:(UICollectionView *)collectionView canMoveItemAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 0)
    {
        return YES;
    }
    
    return NO;
}

- (void)collectionView:(UICollectionView *)collectionView moveItemAtIndexPath:(NSIndexPath *)sourceIndexPath toIndexPath:(NSIndexPath*)destinationIndexPath
{
    if (sourceIndexPath.section == 0 && destinationIndexPath.section == 0)
    {
        UnitObject* unit = [_crewUnits objectAtIndex:sourceIndexPath.item];
        
        [_crewUnits removeObjectAtIndex:sourceIndexPath.item];
        [_crewUnits insertObject:unit atIndex:destinationIndexPath.item];
    }
    else
    {
        [collectionView reloadData];
    }
}

- (BOOL)collectionView:(UICollectionView*)collectionView shouldSelectItemAtIndexPath:(nonnull NSIndexPath *)indexPath
{
    if ([_selection count] < _maxUnitCount)
    {
        return YES;
    }
    else
    {
        return NO;
    }
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    _selectedUnitCount++;
    
    UnitObject* unit = [_crewUnits objectAtIndex:indexPath.item];
    [_selection addObject:unit];
    
    [self updateRecentUnit:unit];
    [self updateSelection];
}

- (void)collectionView:(UICollectionView *)collectionView didDeselectItemAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 0)
    {
        _selectedUnitCount--;
        
        [_selection removeObject:[_crewUnits objectAtIndex:indexPath.item]];
        
        [self updateRecentUnit:NULL];
        [self updateSelection];
    }
}

- (void)updateRecentUnit:(UnitObject*)unit
{
    _currentUnit = unit;

    if (!unit)
    {
        _nameLabel.text = @"";
        _typeLabel.text = @"";
        _battlesLabel.text = @"";
        _primaryLabel.text = @"";
        _secondaryLabel.text = @"";
        [_changeNameButton setHidden:true];
    }
    else
    {
        [_changeNameButton setHidden:false];

        _nameLabel.text = unit.name;
        _typeLabel.text = [NSString stringWithFormat:@"Class: %@", [UnitObject typeToString: unit.type]];
        _battlesLabel.text = [NSString stringWithFormat:@"Battles: %lu", (long)unit.battleCount];
        
        if (unit.type == kUnitScientist)
        {
            _primaryLabel.text = [NSString stringWithFormat:@"Primary: %@", [UnitObject weaponToString:unit.primaryWeaponType]];
            _secondaryLabel.text = [NSString stringWithFormat:@"Secondary: %@", [UnitObject weaponToString:unit.secondaryWeaponType]];
        }
        else
        {
            _primaryLabel.text = @"";
            _secondaryLabel.text = @"";
        }
    }
}

- (void)updateSelection
{
    if (_selectedUnitCount > 0)
    {
        _playButton.hidden = NO;
    }
    else
    {
        _playButton.hidden = YES;
    }
    
    if (_selectedUnitCount == _maxUnitCount)
    {
        self.selectionProgressLabel.textColor = [UIColor whiteColor];
    }
    else
    {
        self.selectionProgressLabel.textColor = [UIColor redColor];
    }
    
    self.selectionProgressLabel.text = [NSString stringWithFormat:@"%li/%li", (long)_selectedUnitCount, (long)_maxUnitCount];
}


- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer
{
    return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    return YES;
}



@end
