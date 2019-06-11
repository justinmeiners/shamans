#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>

typedef enum
{
    kDirectoryData = 0,
    kDirectoryCount
} Directory;

#define LINE_BUFFER_MAX 1024
#define MAX_OS_PATH 1024

extern char* Filepath_Extension(const char* path);
extern void Filepath_Append(char* dest, const char* base, const char* sub);

extern void Filepath_SetDirectory(Directory directory, const char* path);
extern char* Filepath_DataDir();

extern int String_IsEmpty(const char* string);
extern int String_Prefix(const char* str, const char* pre, size_t maxSize);

extern unsigned long String_Hash(const char* string);


extern const int Utils_endian;
extern char Filepath_directories[kDirectoryCount][MAX_OS_PATH];

#define End_IsBig() ((*(char*)&Utils_endian) == 0)
#define End_Swap16(x) ((x>>8) | (x<<8))
#define End_Swap32(x) (((x>>24)&0xff) | ((x<<8)&0xff0000) | ((x>>8)&0xff00) | ((x<<24)&0xff000000))

#define End_ReadLittle16(x) End_IsBig() ? End_Swap16(x) : x
#define End_ReadLittle32(x) End_IsBig() ? End_Swap32(x) : x

#define End_ReadBig16(x) End_IsBig() ? x : End_Swap16(x)
#define End_ReadBig32(x) End_IsBig() ? x : End_Swap32(x)

#endif
