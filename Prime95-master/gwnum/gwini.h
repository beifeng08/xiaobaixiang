// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _GWINI_H
#define _GWINI_H

#ifdef __cplusplus
extern "C" {
#endif

void IniGetString (const char *, const char *, char *, unsigned int, const char *);
void IniSectionGetString (const char *, const char *, const char *, char *, unsigned int, const char *);
void IniGetNthString (const char *, const char *, int, char *, unsigned int, const char *);
void IniSectionGetNthString (const char *, const char *, const char *, int, char *, unsigned int, const char *);

void IniGetTimedString (const char *, const char *, char *, unsigned int, const char *, unsigned int *);
void IniSectionGetTimedString (const char *, const char *, const char *, char *, unsigned int, const char *, unsigned int *);
void IniSectionGetNthTimedString (const char *, const char *, const char *, int, char *, unsigned int, const char *, unsigned int *);

long IniGetInt (const char *, const char *, long);
long IniSectionGetInt (const char *, const char *, const char *, long);
long IniGetTimedInt (const char *, const char *, long, unsigned int *);
long IniSectionGetTimedInt (const char *, const char *, const char *, long, unsigned int *);

int64_t IniGetInt64 (const char *, const char *, int64_t);
int64_t IniSectionGetInt64 (const char *, const char *, const char *, int64_t);
int64_t IniGetTimedInt64 (const char *, const char *, int64_t, unsigned int *);
int64_t IniSectionGetTimedInt64 (const char *, const char *, const char *, int64_t, unsigned int *);

float IniGetFloat (const char *, const char *, float);
float IniSectionGetFloat (const char *, const char *, const char *, float);
float IniGetTimedFloat (const char *, const char *, float, unsigned int *);
float IniSectionGetTimedFloat (const char *, const char *, const char *, float, unsigned int *);

void IniWriteString (const char *, const char *, const char *);
void IniSectionWriteString (const char *, const char *, const char *, const char *);
void IniWriteNthString (const char *, const char *, int, const char *);
void IniSectionWriteNthString (const char *, const char *, const char *, int, const char *);

void IniWriteInt (const char *, const char *, long);
void IniSectionWriteInt (const char *, const char *, const char *, long);

void IniWriteInt64 (const char *, const char *, int64_t);
void IniSectionWriteInt64 (const char *, const char *, const char *, int64_t);

void IniWriteFloat (const char *, const char *, float);
void IniSectionWriteFloat (const char *, const char *, const char *, float);

void IniDelayWrites (const char *);
void IniResumeImmediateWrites (const char *);

extern void (*INI_ERROR_CALLBACK)(const char *, int, const char *);	
									
#define IniSetErrorCallback(n)	(INI_ERROR_CALLBACK = n)

void IniFileReread (const char *);					
void IniAddFileMerge (const char *, const char *, const char *);	

const char *IniSectionGetStringRaw (const char *, const char *, const char *);
const char *IniSectionGetNthStringRaw (const char *, const char *, const char *, int);

#ifdef __cplusplus
}
#endif

#endif
