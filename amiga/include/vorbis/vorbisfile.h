/* Freegish for AmigaOS: the libvorbisfile calls src/game/music.c makes, as
 * stubs that open nothing. Debian's free source has no music (its Ogg
 * files are left out), so there is never a tune to play.
 * The AmiCradle Team. GPL 2 or later, like Freegish. */
#ifndef AMIGA_VORBISFILE_STUB_H
#define AMIGA_VORBISFILE_STUB_H
#include <stddef.h>

#include <ogg/ogg.h>

typedef struct {
	int version;
	int channels;
	long rate;
} vorbis_info;

typedef struct {
	int comments;
} vorbis_comment;

typedef struct {
	size_t (*read_func)(void *ptr, size_t size, size_t nmemb, void *datasource);
	int (*seek_func)(void *datasource, ogg_int64_t offset, int whence);
	int (*close_func)(void *datasource);
	long (*tell_func)(void *datasource);
} ov_callbacks;

typedef struct {
	void *datasource;
} OggVorbis_File;

#define OV_ENOTVORBIS -132

static inline int ov_open_callbacks(void *datasource, OggVorbis_File *vf,
	const char *initial, long ibytes, ov_callbacks callbacks)
{
	(void)datasource; (void)vf; (void)initial; (void)ibytes; (void)callbacks;
	return OV_ENOTVORBIS;
}
static inline int ov_clear(OggVorbis_File *vf) { (void)vf; return 0; }
static inline long ov_read(OggVorbis_File *vf, char *buffer, int length,
	int bigendianp, int word, int sgned, int *bitstream)
{
	(void)vf; (void)buffer; (void)length; (void)bigendianp; (void)word;
	(void)sgned; (void)bitstream;
	return 0;
}
static inline vorbis_info *ov_info(OggVorbis_File *vf, int link)
{
	static vorbis_info vi = { 0, 2, 22050 };
	(void)vf; (void)link;
	return &vi;
}
#endif
