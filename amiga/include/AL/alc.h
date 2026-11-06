/* Freegish for AmigaOS: the OpenAL context calls it makes (amiga/al_lite.c).
 * The AmiCradle Team. GPL 2 or later, like Freegish. */
#ifndef AMIGA_AL_LITE_ALC_H
#define AMIGA_AL_LITE_ALC_H
#include "al.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
typedef char ALCboolean;
typedef char ALCchar;
typedef int ALCint;

ALCdevice *alcOpenDevice(const ALCchar *devicename);
ALCboolean alcCloseDevice(ALCdevice *device);
ALCcontext *alcCreateContext(ALCdevice *device, const ALCint *attrlist);
ALCboolean alcMakeContextCurrent(ALCcontext *context);
void alcDestroyContext(ALCcontext *context);
ALCcontext *alcGetCurrentContext(void);
ALCdevice *alcGetContextsDevice(ALCcontext *context);

#ifdef __cplusplus
}
#endif
#endif
