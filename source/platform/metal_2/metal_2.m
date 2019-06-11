
#include <Metal/Metal.h>
#include "metal_2.h"

@interface Metal2Renderer : NSObject
{
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;
}

- (id)init;
@end

@implementation Metal2Renderer

- (id)init
{
    if ((self = [super init]))
    {
        _device = MTLCreateSystemDefaultDevice();
        _commandQueue = [_device newCommandQueue];
    }
    
    return self;
}

- (void)render:(const Engine*)engine
        camera:(const Frustum*)camera
    renderList:(const RenderList*)renderList
{
    //id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
}

@end


static void Metal2_Render(Renderer* r,
                      const Engine* engine,
                      const Frustum* cam,
                      const RenderList* renderList)
{
    Metal2Renderer* m2 = (__bridge Metal2Renderer *)(r->context);
    [m2 render:engine camera:cam renderList:renderList];
}

void Metal2_Shutdown(Renderer* renderer)
{
    CFBridgingRelease(renderer->context);
}

int Metal2_Init(Renderer* r, const char* shaderDirectory)
{
    Metal2Renderer* m2 = [[Metal2Renderer alloc] init];
    
    if (!m2) {
        return 0;
    }
    
    r->context = (void*)CFBridgingRetain(m2);
    r->render = Metal2_Render;

    return 1;
}

