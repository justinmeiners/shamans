//
//  GameViewController.m
//  Test App
//
//  Created by Justin Meiners on 6/30/16.
//  Copyright © 2016 Justin Meiners. All rights reserved.
//

#import "GameViewController.h"
#import <OpenGLES/ES2/glext.h>
#import <AVFoundation/AVFoundation.h>
#import "UnitObject.h"
#import "DataManager.h"

#include "engine.h"
#include "gl_3.h"
#include "snd_driver_core_audio.h"
#include "ai.h"
#include "human.h"

@interface GameViewController ()
{
    SndDriver _soundDriver;
    Renderer _renderer;
    Player _ai;
    Player _player;
    InputState _inputState;
    UnitInfo* _unitSpawnInfo;
    EAGLContext* _realContext;
    bool _loaded;
}

@property (strong, nonatomic) EAGLContext *context;
@property( strong, nonatomic)EAGLContext* loadingContext;

- (void)tearDownGL;


@end

@implementation GameViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    GLKView *view = (GLKView *)self.view;
    // this is a temporary context which is replaced with the real one we are loading
    view.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.drawableStencilFormat = GLKViewDrawableStencilFormat8;
    
    self.preferredFramesPerSecond = 60;

    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!self.context)
    {
        NSLog(@"Failed to create ES context");
    }
    
    [EAGLContext setCurrentContext:self.context];

    NSString* shaderDirectory = [[NSBundle mainBundle] resourcePath];
    
    if (!Gl2_Init(&_renderer, [shaderDirectory cStringUsingEncoding:NSASCIIStringEncoding]))
    {
        NSLog(@"Failed to setup renderer");
    }
    
    // audio
    [self configureAVAudioSession];
    
    if (!SndDriver_CoreAudio_Init(&_soundDriver, SND_DRIVER_CORE_AUDIO_RATE_DEFAULT))
    {
        NSLog(@"Failed to setup core audio driver");
    }
    
    _unitSpawnInfo = [[DataManager sharedManager] allocUnitTransaction:_selectedUnits];
    
    Human_Init(&_player, _unitSpawnInfo, (int)_selectedUnits.count);
    Ai_Init(&_ai);

    InputState_Init(&_inputState);

    EngineSettings engineSettings;
    engineSettings.dataPath = [[[NSBundle mainBundle] pathForResource:@"data" ofType:NULL] cStringUsingEncoding:NSASCIIStringEncoding];
    engineSettings.levelPath = [_levelPath cStringUsingEncoding:NSUTF8StringEncoding];
    engineSettings.inputConfig = kInputConfigMultitouch;
    engineSettings.guiWidth = 1024;
    engineSettings.guiHeight = 768;
    engineSettings.renderWidth = self.view.bounds.size.width;
    engineSettings.renderHeight = self.view.bounds.size.height;
    engineSettings.renderScaleFactor = self.view.contentScaleFactor;
    
    _loaded = false;
    
    // init engine on background thread
    // so that loading doesn't trigger the watchdog or lock up the device
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [EAGLContext setCurrentContext:self.context];
        
        if (!Engine_Init(&g_engine, &_renderer, &_soundDriver, engineSettings, &_player, &_ai))
        {
            NSLog(@"Fatal engine error");
        }
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            [_loadingLabel setHidden:YES];
            
            GLKView* glView = (GLKView *)self.view;
            glView.context = self.context;
            _loaded = true;

        });
    });
    
    UILongPressGestureRecognizer* longGesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longPress:)];
    [self.view addGestureRecognizer:longGesture];
}

- (void)longPress:(UILongPressGestureRecognizer*)gesture
{
    // for level screenshots
    if (gesture.state == UIGestureRecognizerStateBegan)
    {
        g_engine.guiSystem.drawEnabled = !g_engine.guiSystem.drawEnabled;
    }
}

- (void)dealloc
{
    free(_unitSpawnInfo);
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context)
        {
            [EAGLContext setCurrentContext:nil];
        }
        
        self.context = nil;
    }
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)configureAVAudioSession
{
    // Get your app's audioSession singleton object
    AVAudioSession *session = [AVAudioSession sharedInstance];
    
    // Error handling
    BOOL success;
    NSError *error;
    
    // set the audioSession category.
    // Needs to be Record or PlayAndRecord to use audioRouteOverride:
    
    success = [session setCategory:AVAudioSessionCategoryPlayback
                             error:&error];
    
    if (!success) {
        NSLog(@"AVAudioSession error setting category:%@",error);
    }
    
    // Set the audioSession override
    success = [session overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker
                                         error:&error];
    if (!success) {
        NSLog(@"AVAudioSession error overrideOutputAudioPort:%@",error);
    }
    
    // Activate the audio session
    success = [session setActive:YES error:&error];
    if (!success) {
        NSLog(@"AVAudioSession error activating: %@",error);
    }
    else {
        NSLog(@"AudioSession active");
    }
    
}

- (void)tearDownGL
{
    Engine_Shutdown(&g_engine);
    [EAGLContext setCurrentContext:self.context];

}

#pragma mark - GLKView and GLKViewController delegate methods


- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if (!_loaded)
        return;
    
    EngineState state = Engine_Tick(&g_engine, &_inputState);
    Engine_Render(&g_engine);
    
    if (state == kEngineStateEnd)
    {
        if (g_engine.result == kEngineResultVictory)
        {            
            Player* localPlayer = Engine_LocalPlayer(&g_engine);
            
            UnitInfo resultInfo[SCENE_SYSTEM_UNITS_MAX];
            NSInteger unitCount = (NSInteger)Player_GetResultUnitInfos(localPlayer, resultInfo);
            
            [_gameDelegate gameViewControllerFinished:self withResult:g_engine.result unitInfo:resultInfo unitCount:unitCount];
        }
        else
        {
            [_gameDelegate gameViewControllerFinished:self withResult:g_engine.result unitInfo:NULL unitCount:0];
        }
    }
}

- (void)handleTouches:(UIEvent*)event
{
    int touchCount = 0;
    
    NSSet* allTouches = [event allTouches];
    for (UITouch* touch in allTouches)
    {
        CGPoint touchLocation = [touch locationInView:nil];
        
        _inputState.touches[touchCount].identifier = (int)touch.hash;
        
        _inputState.touches[touchCount].x = (touchLocation.x / self.view.bounds.size.width) * 2.0f - 1.0f;
        _inputState.touches[touchCount].y = -((touchLocation.y / self.view.bounds.size.height) * 2.0f - 1.0f);
        
        InputPhase phase;
        
        switch (touch.phase)
        {
            case UITouchPhaseBegan:
                phase = kInputPhaseBegin;
                break;
            case UITouchPhaseCancelled:
            case UITouchPhaseEnded:
                phase = kInputPhaseEnd;
                break;
            case UITouchPhaseStationary:
            case UITouchPhaseMoved:
                phase = kInputPhaseMove;
                break;
        }
        
        _inputState.touches[touchCount].phase = phase;
        ++touchCount;
    }
    
    _inputState.touchCount = touchCount;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
    [self handleTouches:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self handleTouches:event];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
    [self handleTouches:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self handleTouches:event];
}

@end
