/* The OpenAL 1.1 calls Freegish makes (see include/AL/al.h), on SDL 2's
 * audio: there is no OpenAL for AmigaOS 3.
 *
 * Output: 16-bit signed stereo at 22050 Hz, through SDL_OpenAudioDevice
 * (SDL converts if the device wants something else).
 *
 * Buffers keep their samples as 16-bit (8-bit ones are widened at
 * alBufferData), mono or stereo, at their own rate. Sources play one
 * buffer, looping or not, or a queue of them (the music's stream), with
 * their gain and pitch; each step through a buffer is 16.16 fixed point,
 * so the 68k only adds up integers.
 *
 * Positions: the listener's position and orientation and a source's
 * position give the source's distance and side; the distance attenuates as
 * AL_INVERSE_DISTANCE_CLAMPED (the default model) with the source's
 * reference distance, the side pans between the speakers. Velocity
 * (Doppler) is not used. Gains and pans are worked out once a source is
 * changed or the listener moves, not for every sample.
 *
 * The AmiCradle Team. GPL 2 or later, like Freegish. */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "AL/al.h"
#include "AL/alc.h"

#define OUT_RATE 22050
#define MAX_BUFFERS 256
#define MAX_SOURCES 64
#define MAX_QUEUE 8
#define SEG 512

struct albuf {
	int used;
	Sint16 *data;
	Uint32 frames;
	int channels;
	int freq;
};

struct alsrc {
	int used;
	ALuint buffer;		/* static buffer, 0: none */
	ALuint queue[MAX_QUEUE];
	int nqueue;		/* buffers queued */
	int qpos;		/* the one playing; those before it are processed */
	int looping;
	int relative;
	ALenum state;
	float gain, pitch, refdist;
	float pos[3];
	Uint32 frame, frac;	/* place in the playing buffer, 16.16 */
	int gl, gr;		/* 0..256 each, worked out by update_gains */
};

static struct albuf bufs[MAX_BUFFERS];
static struct alsrc srcs[MAX_SOURCES];
static float lpos[3], lat[3] = { 0, 0, -1 }, lup[3] = { 0, 1, 0 };
static SDL_AudioDeviceID dev;
static Sint32 accl[SEG], accr[SEG];
static int dummy_device, dummy_context;

static void lock(void)
{
	if (dev)
		SDL_LockAudioDevice(dev);
}

static void unlock(void)
{
	if (dev)
		SDL_UnlockAudioDevice(dev);
}

static struct albuf *getbuf(ALuint b)
{
	return (b > 0 && b <= MAX_BUFFERS && bufs[b - 1].used) ? &bufs[b - 1] : NULL;
}

static struct alsrc *getsrc(ALuint s)
{
	return (s > 0 && s <= MAX_SOURCES && srcs[s - 1].used) ? &srcs[s - 1] : NULL;
}

/* the source's left and right gains from its gain, distance and side */
static void update_gains(struct alsrc *s)
{
	float g = s->gain, pan = 0.0f;
	float d[3], dist;
	int i;

	if (g < 0.0f)
		g = 0.0f;
	for (i = 0; i < 3; i++)
		d[i] = s->relative ? s->pos[i] : s->pos[i] - lpos[i];
	dist = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
	if (dist > 0.0001f) {
		/* right = at x up */
		float rx = lat[1] * lup[2] - lat[2] * lup[1];
		float ry = lat[2] * lup[0] - lat[0] * lup[2];
		float rz = lat[0] * lup[1] - lat[1] * lup[0];
		float rl = sqrtf(rx * rx + ry * ry + rz * rz);
		if (rl > 0.0001f)
			pan = (d[0] * rx + d[1] * ry + d[2] * rz) / (dist * rl);
		if (s->refdist > 0.0f && dist > s->refdist)
			g *= s->refdist / dist;
	}
	if (g > 1.0f)
		g = 1.0f;
	s->gl = (int)(g * (pan > 0.0f ? 1.0f - pan : 1.0f) * 256.0f);
	s->gr = (int)(g * (pan < 0.0f ? 1.0f + pan : 1.0f) * 256.0f);
}

/* the buffer a source is playing now, or NULL */
static struct albuf *current(struct alsrc *s)
{
	if (s->nqueue)
		return s->qpos < s->nqueue ? getbuf(s->queue[s->qpos]) : NULL;
	return getbuf(s->buffer);
}

static void mix_source(struct alsrc *s, int n)
{
	int i = 0;

	while (i < n && s->state == AL_PLAYING) {
		struct albuf *b = current(s);
		Uint32 step;
		const Sint16 *d;
		int gl = s->gl, gr = s->gr;

		if (!b || !b->data || b->frames == 0) {
			if (s->nqueue && s->qpos < s->nqueue) {
				s->qpos++;
				continue;
			}
			s->state = AL_STOPPED;
			break;
		}
		step = (Uint32)((double)b->freq / OUT_RATE * (s->pitch > 0.0f ? s->pitch : 1.0f) * 65536.0);
		if (step == 0)
			step = 1;
		d = b->data;
		for (; i < n; i++) {
			Sint32 l, r;
			if (s->frame >= b->frames)
				break;
			if (b->channels == 2) {
				l = d[s->frame * 2];
				r = d[s->frame * 2 + 1];
			} else
				l = r = d[s->frame];
			accl[i] += (l * gl) >> 8;
			accr[i] += (r * gr) >> 8;
			s->frac += step;
			s->frame += s->frac >> 16;
			s->frac &= 0xFFFF;
		}
		if (s->frame >= b->frames) {
			s->frame = 0;
			s->frac = 0;
			if (s->nqueue) {
				s->qpos++;
				if (s->qpos >= s->nqueue)
					s->state = AL_STOPPED;
			} else if (!s->looping)
				s->state = AL_STOPPED;
		}
	}
}

static void SDLCALL mix_callback(void *user, Uint8 *stream, int len)
{
	Sint16 *out = (Sint16 *)stream;
	int frames = len / 4, i, c;
	(void)user;

	while (frames > 0) {
		int n = frames > SEG ? SEG : frames;
		memset(accl, 0, sizeof(Sint32) * n);
		memset(accr, 0, sizeof(Sint32) * n);
		for (c = 0; c < MAX_SOURCES; c++)
			if (srcs[c].used && srcs[c].state == AL_PLAYING)
				mix_source(&srcs[c], n);
		for (i = 0; i < n; i++) {
			Sint32 l = accl[i], r = accr[i];
			*out++ = (Sint16)(l > 32767 ? 32767 : l < -32768 ? -32768 : l);
			*out++ = (Sint16)(r > 32767 ? 32767 : r < -32768 ? -32768 : r);
		}
		frames -= n;
	}
}

/* ---- context ---- */

ALCdevice *alcOpenDevice(const ALCchar *devicename)
{
	SDL_AudioSpec want, have;
	(void)devicename;

	if (dev)
		return (ALCdevice *)&dummy_device;
	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		return NULL;
	SDL_zero(want);
	want.freq = OUT_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = mix_callback;
	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (!dev)
		return NULL;
	SDL_PauseAudioDevice(dev, 0);
	return (ALCdevice *)&dummy_device;
}

ALCboolean alcCloseDevice(ALCdevice *device)
{
	int i;
	(void)device;
	if (dev) {
		SDL_CloseAudioDevice(dev);
		dev = 0;
	}
	for (i = 0; i < MAX_BUFFERS; i++) {
		free(bufs[i].data);
		bufs[i].data = NULL;
		bufs[i].used = 0;
	}
	memset(srcs, 0, sizeof(srcs));
	return AL_TRUE;
}

ALCcontext *alcCreateContext(ALCdevice *device, const ALCint *attrlist)
{
	(void)attrlist;
	return device ? (ALCcontext *)&dummy_context : NULL;
}

ALCboolean alcMakeContextCurrent(ALCcontext *context)
{
	(void)context;
	return AL_TRUE;
}

void alcDestroyContext(ALCcontext *context)
{
	(void)context;
}

ALCcontext *alcGetCurrentContext(void)
{
	return dev ? (ALCcontext *)&dummy_context : NULL;
}

ALCdevice *alcGetContextsDevice(ALCcontext *context)
{
	return context ? (ALCdevice *)&dummy_device : NULL;
}

/* ---- buffers ---- */

void alGenBuffers(ALsizei n, ALuint *buffers)
{
	int i, j = 0;
	for (i = 0; i < MAX_BUFFERS && j < n; i++)
		if (!bufs[i].used) {
			memset(&bufs[i], 0, sizeof(bufs[i]));
			bufs[i].used = 1;
			buffers[j++] = (ALuint)(i + 1);
		}
	while (j < n)
		buffers[j++] = 0;
}

void alDeleteBuffers(ALsizei n, const ALuint *buffers)
{
	int i;
	lock();
	for (i = 0; i < n; i++) {
		struct albuf *b = getbuf(buffers[i]);
		if (b) {
			free(b->data);
			memset(b, 0, sizeof(*b));
		}
	}
	unlock();
}

void alBufferData(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq)
{
	struct albuf *b = getbuf(buffer);
	int channels = (format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16) ? 2 : 1;
	int eight = (format == AL_FORMAT_MONO8 || format == AL_FORMAT_STEREO8);
	Uint32 samples, i;
	Sint16 *d;

	if (!b || size <= 0)
		return;
	samples = eight ? (Uint32)size : (Uint32)size / 2;
	d = (Sint16 *)malloc(samples * sizeof(Sint16));
	if (!d)
		return;
	if (eight)
		for (i = 0; i < samples; i++)
			d[i] = (Sint16)(((int)((const Uint8 *)data)[i] - 128) << 8);
	else
		memcpy(d, data, samples * sizeof(Sint16));	/* native order, as OpenAL wants */
	lock();
	free(b->data);
	b->data = d;
	b->channels = channels;
	b->frames = samples / channels;
	b->freq = freq > 0 ? freq : OUT_RATE;
	unlock();
}

/* ---- sources ---- */

void alGenSources(ALsizei n, ALuint *sources)
{
	int i, j = 0;
	lock();
	for (i = 0; i < MAX_SOURCES && j < n; i++)
		if (!srcs[i].used) {
			memset(&srcs[i], 0, sizeof(srcs[i]));
			srcs[i].used = 1;
			srcs[i].state = AL_INITIAL;
			srcs[i].gain = 1.0f;
			srcs[i].pitch = 1.0f;
			srcs[i].refdist = 1.0f;
			update_gains(&srcs[i]);
			sources[j++] = (ALuint)(i + 1);
		}
	unlock();
	while (j < n)
		sources[j++] = 0;
}

void alDeleteSources(ALsizei n, const ALuint *sources)
{
	int i;
	lock();
	for (i = 0; i < n; i++) {
		struct alsrc *s = getsrc(sources[i]);
		if (s)
			memset(s, 0, sizeof(*s));
	}
	unlock();
}

void alSourcei(ALuint source, ALenum param, ALint value)
{
	struct alsrc *s = getsrc(source);
	if (!s)
		return;
	lock();
	switch (param) {
	case AL_BUFFER:
		s->buffer = (ALuint)value;
		s->nqueue = s->qpos = 0;
		s->frame = s->frac = 0;
		break;
	case AL_LOOPING:
		s->looping = value != 0;
		break;
	case AL_SOURCE_RELATIVE:
		s->relative = value != 0;
		update_gains(s);
		break;
	}
	unlock();
}

void alSourcef(ALuint source, ALenum param, ALfloat value)
{
	struct alsrc *s = getsrc(source);
	if (!s)
		return;
	lock();
	switch (param) {
	case AL_GAIN:
		s->gain = value;
		break;
	case AL_PITCH:
		s->pitch = value;
		break;
	case AL_REFERENCE_DISTANCE:
		s->refdist = value;
		break;
	}
	update_gains(s);
	unlock();
}

void alSource3f(ALuint source, ALenum param, ALfloat v1, ALfloat v2, ALfloat v3)
{
	ALfloat v[3];
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	alSourcefv(source, param, v);
}

void alSourcefv(ALuint source, ALenum param, const ALfloat *values)
{
	struct alsrc *s = getsrc(source);
	if (!s || param != AL_POSITION)
		return;	/* velocity: no Doppler here */
	lock();
	memcpy(s->pos, values, sizeof(s->pos));
	update_gains(s);
	unlock();
}

void alGetSourcei(ALuint source, ALenum param, ALint *value)
{
	struct alsrc *s = getsrc(source);
	if (!value)
		return;
	*value = 0;
	if (!s)
		return;
	lock();
	switch (param) {
	case AL_SOURCE_STATE:
		*value = s->state;
		break;
	case AL_BUFFERS_QUEUED:
		*value = s->nqueue;
		break;
	case AL_BUFFERS_PROCESSED:
		*value = s->state == AL_STOPPED ? s->nqueue : s->qpos;
		break;
	case AL_BUFFER:
		*value = (ALint)s->buffer;
		break;
	case AL_LOOPING:
		*value = s->looping;
		break;
	}
	unlock();
}

void alSourcePlay(ALuint source)
{
	struct alsrc *s = getsrc(source);
	if (!s)
		return;
	lock();
	if (s->state != AL_PLAYING) {
		if (s->state != AL_PAUSED) {
			s->frame = s->frac = 0;
			if (s->nqueue && s->qpos >= s->nqueue)
				s->qpos = 0;
		}
		s->state = AL_PLAYING;
	} else if (!s->nqueue) {
		s->frame = s->frac = 0;	/* playing again restarts it */
	}
	unlock();
}

void alSourceStop(ALuint source)
{
	struct alsrc *s = getsrc(source);
	if (!s)
		return;
	lock();
	s->state = AL_STOPPED;
	s->frame = s->frac = 0;
	unlock();
}

void alSourcePause(ALuint source)
{
	struct alsrc *s = getsrc(source);
	if (!s)
		return;
	lock();
	if (s->state == AL_PLAYING)
		s->state = AL_PAUSED;
	unlock();
}

void alSourceQueueBuffers(ALuint source, ALsizei n, const ALuint *buffers)
{
	struct alsrc *s = getsrc(source);
	int i;
	if (!s)
		return;
	lock();
	s->buffer = 0;
	for (i = 0; i < n && s->nqueue < MAX_QUEUE; i++)
		s->queue[s->nqueue++] = buffers[i];
	unlock();
}

void alSourceUnqueueBuffers(ALuint source, ALsizei n, ALuint *buffers)
{
	struct alsrc *s = getsrc(source);
	int i, done;
	if (!s)
		return;
	lock();
	done = s->state == AL_STOPPED ? s->nqueue : s->qpos;
	if (n > done)
		n = done;
	for (i = 0; i < n; i++)
		buffers[i] = s->queue[i];
	memmove(s->queue, s->queue + n, (size_t)(s->nqueue - n) * sizeof(ALuint));
	s->nqueue -= n;
	s->qpos -= n;
	if (s->qpos < 0)
		s->qpos = 0;
	unlock();
}

/* ---- listener and the rest ---- */

void alListenerfv(ALenum param, const ALfloat *values)
{
	int i;
	if (param == AL_POSITION) {
		lock();
		memcpy(lpos, values, sizeof(lpos));
	} else if (param == AL_ORIENTATION) {
		lock();
		memcpy(lat, values, sizeof(lat));
		memcpy(lup, values + 3, sizeof(lup));
	} else
		return;
	for (i = 0; i < MAX_SOURCES; i++)
		if (srcs[i].used)
			update_gains(&srcs[i]);
	unlock();
}

void alListenerf(ALenum param, ALfloat value)
{
	(void)param;
	(void)value;
}

void alDistanceModel(ALenum model)
{
	(void)model;
}

ALenum alGetError(void)
{
	return AL_NO_ERROR;
}
