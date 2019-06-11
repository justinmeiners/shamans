
#include "skel_anim.h"
#include <assert.h>
#include "platform.h"

// \[([-+]?[0-9]*\.[0-9]+|[0-9]+), ([-+]?[0-9]*\.[0-9]+|[0-9]+), (0.000000)\]

int SkelAnim_Init(SkelAnim* anim,
                  unsigned short jointCount,
                  unsigned short frameCount,
                  unsigned short markerCount)
{
    assert(anim);
    
    anim->jointCount = jointCount;
    anim->frameCount = frameCount;
    anim->markerCount = markerCount;
    
    if (anim->markerCount > 0)
    {
        anim->markers = malloc(sizeof(SkelMarker) * markerCount);
    }
    else
    {
        anim->markers = NULL;
    }
    
    anim->frames = malloc(sizeof(SeklFrame) * frameCount);
    if (!anim->frames)
        return 0;
    
    anim->jointRotations = malloc(sizeof(Quat) * jointCount * frameCount);
    if (!anim->jointRotations)
    {
        free(anim->frames);
        return 0;
    }
    
    for (int i = 0; i < frameCount; ++i)
        anim->frames[i].markerIndex = -1;
    
    return 1;
}

void SkelAnim_Shutdown(SkelAnim* anim)
{
    if (!anim)
        return;
    
    if (anim->frames)
        free(anim->frames);
    
    if (anim->jointRotations)
        free(anim->jointRotations);
    
    if (anim->markers)
        free(anim->markers);
}

static int SkelAnim_FromSKANIM(SkelAnim* anim, FILE* file)
{
    char lineBuffer[LINE_BUFFER_MAX];
    int readInfo = 0;
    
    short jointCount = -1;
    short frameCount = -1;
    short markerCount = -1;
    int version = -1;
    
    anim->framesPerSecond = 30;
    
    while (!feof(file))
    {
        fgets(lineBuffer, LINE_BUFFER_MAX, file);
        char* savePtr;
        char* command = strtok_r(lineBuffer, ":", &savePtr);
        char* data = strtok_r(NULL, ":", &savePtr);
        
        if (String_Prefix(command, "#", LINE_BUFFER_MAX))
        {
            // comment
        }
        else if (strstr(command, "version"))
        {
            version = atoi(data);
            
            if (version > 3)
            {
                return 0;
            }
        }
        else if (strstr(command, "frame_count"))
        {
            frameCount = atoi(data);
            
            if (version == 1)
            {
                frameCount = frameCount - 1;
            }
        }
        else if (strstr(command, "joint_count"))
        {
            jointCount = atoi(data);
        }
        else if (strstr(command, "marker_count"))
        {
            markerCount = atoi(data);
        }
        else if (strstr(command, "frames"))
        {
            if (!readInfo)
            {
                assert(jointCount != -1);
                assert(frameCount != -1);
                assert(markerCount != -1);
                
                SkelAnim_Init(anim, jointCount, frameCount, markerCount);
                readInfo = 1;
            }
            
            if (version < 3)
            {
                for (int i = 0; i < frameCount; ++i)
                {
                    SeklFrame* frame = &anim->frames[i];
                    frame->rootOffset = Vec3_Zero;
                    
                    size_t frameDataStart = jointCount * i;
                    
                    for (int j = 0; j < jointCount; ++j)
                    {
                        Quat* rotation = anim->jointRotations + (frameDataStart + j);
                        
                        fgets(lineBuffer, LINE_BUFFER_MAX, file);
                        sscanf(lineBuffer, "%f, %f, %f, %f", &rotation->w, &rotation->x, &rotation->y, &rotation->z);
                    }
                }
            }
            else
            {
                for (int i = 0; i < frameCount; ++i)
                {
                    SeklFrame* frame = &anim->frames[i];
                    fgets(lineBuffer, LINE_BUFFER_MAX, file);
                    sscanf(lineBuffer, "[%f, %f, %f]", &frame->rootOffset.x, &frame->rootOffset.y, &frame->rootOffset.z);
                    
                    size_t frameDataStart = jointCount * i;
                    
                    for (int j = 0; j < jointCount; ++j)
                    {
                        Quat* rotation = anim->jointRotations + (frameDataStart + j);
                        
                        fgets(lineBuffer, LINE_BUFFER_MAX, file);
                        sscanf(lineBuffer, "[%f, %f, %f, %f]", &rotation->w, &rotation->x, &rotation->y, &rotation->z);
                    }
                }
            }
        }
        else if (strstr(command, "markers"))
        {
            for (int i = 0; i < markerCount; ++i)
            {
                SkelMarker* marker = &anim->markers[i];
                
                fgets(lineBuffer, LINE_BUFFER_MAX, file);
                sscanf(lineBuffer, "( %s ), %hd", marker->name, &marker->frame);
                                
                anim->frames[marker->frame].markerIndex = i;
            }
        }
    }
    
    return 1;
}

int SkelAnim_FromPath(SkelAnim* anim, const char* path)
{
    FILE* file = fopen(path, "r");
    
    if (!file) return 0;
    
    int status = 0;
    
    if (strcmp(Filepath_Extension(path), "skanim") == 0)
    {
        status = SkelAnim_FromSKANIM(anim, file);
    }
    
    fclose(file);
    
    if (status)
        return 1;
    
    return 0;
}

int SkelAnim_FindMarker(SkelAnim* anim, const char* markerName)
{
    for (int i = 0; i < anim->markerCount; ++i)
    {
        if (strcmp(anim->markers[i].name, markerName) == 0)
            return i;
    }
    
    return -1;
}

void SkelAnimatorInfo_Clear(SkelAnimatorInfo* info)
{
    info->marker = NULL;
    info->finishedAnim = 0;
    info->finishedTransition = 0;
}

int SkelAnimator_Init(SkelAnimator* animator, Skel* skel)
{
    if (!animator)
        return 0;
    
    animator->skel = skel;
    animator->anim = NULL;
    animator->targetAnim = NULL;
    animator->frame = 0;
    animator->targetAnim = 0;
    animator->transitionFrame = 0;
    animator->transitionFrameCount = 0;
    
    animator->playbackRate = 1;
    
    animator->enableInterp = 1;
    return 1;
}

void SkelAnimator_Shutdown(SkelAnimator* animator)
{
    
}

static void SkelAnimator_ReplaceAnim(SkelAnimator* animator,
                                     const SkelAnim* anim,
                                     int startFrame)
{
    animator->anim = anim;
    animator->targetAnim = NULL;
    animator->targetStartFrame = 0;
    animator->frame = startFrame;
    animator->subFrame = 0;
    
    size_t rotationDataOffset = startFrame * anim->jointCount;
    
    for (int i = 0; i < animator->skel->jointCount; ++i)
        animator->skel->joints[i].rotation = anim->jointRotations[rotationDataOffset + i];
}

int SkelAnimator_SetAnim(SkelAnimator* animator,
                         const SkelAnim* anim,
                         int startFrame,
                         int transitionFrameCount)
{
    if (!anim)
        return 0;
    
    if (animator->anim == anim || (animator->targetAnim == anim && transitionFrameCount != 0))
        return 0;
    
    if (anim->jointCount != animator->skel->jointCount)
    {
        printf("anim bones: %i do not match skel bones %i\n", anim->jointCount, animator->skel->jointCount);
        return 0;
    }
    
    if (animator->anim == NULL || transitionFrameCount < 1)
    {
        SkelAnimator_ReplaceAnim(animator, anim, startFrame);
    }
    else
    {
        animator->targetAnim = anim;
        animator->targetStartFrame = startFrame;
        animator->transitionFrameCount = transitionFrameCount;
        animator->transitionFrame = 0;
    }
    
    return 1;
}

int SkelAnimator_InTransition(SkelAnimator* animator)
{
    assert(animator);
    return (animator->targetAnim != NULL);
}

void SkelAnimator_RestartAnim(SkelAnimator* animator)
{
    assert(animator);
    animator->frame = 0;
    animator->subFrame = 0;
}

static void SkelAnimator_TickAnim(SkelAnimator* animator, Skel* skel, SkelAnimatorInfo* frameInfo)
{
    assert(animator && skel && frameInfo);
    const SkelAnim* anim = animator->anim;
    
    if (!anim || anim->frameCount < 1)
        return;
    
    int frameLength = (60 / anim->framesPerSecond) / animator->playbackRate;
    
    int currentIndex = animator->frame;
    int nextIndex = (currentIndex + 1) % anim->frameCount;
    
    size_t currentDataStart = currentIndex * anim->jointCount;
    size_t nextDataStart = nextIndex * anim->jointCount;
    
    const SeklFrame* current = anim->frames + currentIndex;
    const SeklFrame* next = anim->frames + nextIndex;
    
    if (animator->enableInterp && frameLength != 1)
    {
        float interp = animator->subFrame / (float)frameLength;
        
        for (int i = 0; i < skel->jointCount; ++i)
        {
            Quat from = anim->jointRotations[currentDataStart + i];
            Quat to = anim->jointRotations[nextDataStart + i];
            skel->joints[i].rotation = Quat_Slerp(from, to, interp);
        }
        
        skel->offset = Vec3_Lerp(current->rootOffset, next->rootOffset, interp);
    }
    else
    {
        // no interploation necessary 1 to 1 frame count
        for (int i = 0; i < skel->jointCount; ++i)
            skel->joints[i].rotation = anim->jointRotations[currentDataStart + i];
        
        skel->offset = current->rootOffset;
    }
    
    ++animator->subFrame;
    
    if (animator->subFrame >= frameLength)
    {
        ++animator->frame;
        
        if (animator->frame >= anim->frameCount)
            animator->frame = 0;
        
        animator->subFrame = 0;
    }
    
    if (animator->subFrame == 0 && current->markerIndex != -1)
    {
        frameInfo->marker = anim->markers + current->markerIndex;
    }
    else
    {
        frameInfo->marker = NULL;
    }
    
    if (animator->subFrame == 0 && currentIndex == animator->anim->frameCount - 1)
        frameInfo->finishedAnim = 1;
}


SkelAnimatorInfo SkelAnimator_Tick(SkelAnimator* animator)
{
    assert(animator);
    Skel* skel = animator->skel;
    
    SkelAnimatorInfo info;
    SkelAnimatorInfo_Clear(&info);
    
    if (animator->targetAnim != NULL)
    {
        // calculate transition frames by slerping joints
        const SkelAnim* currentAnim = animator->anim;
        const SkelAnim* targetAnim = animator->targetAnim;
        
        float interp = animator->transitionFrame / (float)animator->transitionFrameCount;
        
        size_t previousDataStart = animator->frame * currentAnim->jointCount;
        size_t nextDataStart = animator->targetStartFrame * currentAnim->jointCount;
        
        int maxJoints = MIN(skel->jointCount, MIN(animator->anim->jointCount, animator->targetAnim->jointCount));
        
        for (int i = 0; i < maxJoints; ++i)
        {
            Quat from = currentAnim->jointRotations[previousDataStart + i];
            Quat to = targetAnim->jointRotations[nextDataStart + i];
            skel->joints[i].rotation = Quat_Slerp(from, to, interp);
        }
        
        // lerp offset as well
        Vec3 currentOffset = currentAnim->frames[animator->frame].rootOffset;
        Vec3 targetOffset = targetAnim->frames[animator->targetStartFrame].rootOffset;
        
        skel->offset = Vec3_Lerp(currentOffset, targetOffset, interp);
        
        ++animator->transitionFrame;
        
        if (animator->transitionFrame == animator->transitionFrameCount - 1)
        {
            SkelAnimator_ReplaceAnim(animator, animator->targetAnim, 0);
            info.finishedTransition = 1;
        }
    }
    else
    {
        SkelAnimator_TickAnim(animator, skel, &info);
    }
    
    return info;
}
