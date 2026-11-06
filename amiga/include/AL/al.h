/* Freegish for AmigaOS: the OpenAL 1.1 calls it makes, on SDL 2's audio
 * (amiga/al_lite.c): there is no OpenAL for AmigaOS 3. Names and values are
 * OpenAL's. The AmiCradle Team. GPL 2 or later, like Freegish. */
#ifndef AMIGA_AL_LITE_AL_H
#define AMIGA_AL_LITE_AL_H
#ifdef __cplusplus
extern "C" {
#endif

typedef char ALboolean;
typedef char ALchar;
typedef signed char ALbyte;
typedef unsigned char ALubyte;
typedef short ALshort;
typedef unsigned short ALushort;
typedef int ALint;
typedef unsigned int ALuint;
typedef int ALsizei;
typedef int ALenum;
typedef float ALfloat;
typedef double ALdouble;
typedef void ALvoid;

#define AL_NONE 0
#define AL_FALSE 0
#define AL_TRUE 1
#define AL_SOURCE_RELATIVE 0x202
#define AL_PITCH 0x1003
#define AL_POSITION 0x1004
#define AL_VELOCITY 0x1006
#define AL_LOOPING 0x1007
#define AL_BUFFER 0x1009
#define AL_GAIN 0x100A
#define AL_ORIENTATION 0x100F
#define AL_SOURCE_STATE 0x1010
#define AL_INITIAL 0x1011
#define AL_PLAYING 0x1012
#define AL_PAUSED 0x1013
#define AL_STOPPED 0x1014
#define AL_BUFFERS_QUEUED 0x1015
#define AL_BUFFERS_PROCESSED 0x1016
#define AL_REFERENCE_DISTANCE 0x1020
#define AL_ROLLOFF_FACTOR 0x1021
#define AL_MAX_DISTANCE 0x1023
#define AL_FORMAT_MONO8 0x1100
#define AL_FORMAT_MONO16 0x1101
#define AL_FORMAT_STEREO8 0x1102
#define AL_FORMAT_STEREO16 0x1103
#define AL_NO_ERROR 0
#define AL_INVALID_NAME 0xA001
#define AL_INVERSE_DISTANCE 0xD001
#define AL_INVERSE_DISTANCE_CLAMPED 0xD002

void alGenBuffers(ALsizei n, ALuint *buffers);
void alDeleteBuffers(ALsizei n, const ALuint *buffers);
void alBufferData(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq);
void alGenSources(ALsizei n, ALuint *sources);
void alDeleteSources(ALsizei n, const ALuint *sources);
void alSourcei(ALuint source, ALenum param, ALint value);
void alSourcef(ALuint source, ALenum param, ALfloat value);
void alSource3f(ALuint source, ALenum param, ALfloat v1, ALfloat v2, ALfloat v3);
void alSourcefv(ALuint source, ALenum param, const ALfloat *values);
void alGetSourcei(ALuint source, ALenum param, ALint *value);
void alSourcePlay(ALuint source);
void alSourceStop(ALuint source);
void alSourcePause(ALuint source);
void alSourceQueueBuffers(ALuint source, ALsizei n, const ALuint *buffers);
void alSourceUnqueueBuffers(ALuint source, ALsizei n, ALuint *buffers);
void alListenerfv(ALenum param, const ALfloat *values);
void alListenerf(ALenum param, ALfloat value);
void alDistanceModel(ALenum model);
ALenum alGetError(void);

#ifdef __cplusplus
}
#endif
#endif
