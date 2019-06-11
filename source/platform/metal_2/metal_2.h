
#ifndef METAL_2_H
#define METAL_2_H

#include <stdio.h>
#include "engine.h"

/*
 
 [Device Compatibility](https://developer.apple.com/library/archive/documentation/DeviceInformation/Reference/iOSDeviceCompatibility/HardwareGPUInformation/HardwareGPUInformation.html)
 
 */

extern int Metal2_Init(Renderer* renderer, const char* shaderDirectory);
extern void Metal2_Shutdown(Renderer* renderer);

#endif
