/* getenv() for Freegish on AmigaOS (-Wl,--wrap=getenv), from the GNU Robbo
 * port. The game keeps its settings, scores, players and its own levels in
 * $HOME/.freegish; here $HOME is the drawer's user/, made at the first
 * start. Every other name is libnix's getenv's.
 * The AmiCradle Team. GPL 2 or later, like Freegish. */
#include <string.h>
#include <sys/stat.h>

char *__real_getenv(const char *name);

char *__wrap_getenv(const char *name)
{
	static int made;
	if (name && strcmp(name, "HOME") == 0) {
		if (!made) {
			mkdir("PROGDIR:user", 0755);
			made = 1;
		}
		return "PROGDIR:user";
	}
	return __real_getenv(name);
}
