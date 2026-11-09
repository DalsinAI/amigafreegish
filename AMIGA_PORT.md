Freegish 2.0.0 for AmigaOS 3.x
==============================

Freegish (the FreeGish project, https://github.com/freegish/freegish), the
free version of Cryptic Sea's Gish: a ball of tar that sticks to walls,
slides, gets heavy and jumps its way through sewers, caves and worse, in a
physics-driven platformer, with a versus mode for up to four players. This
is the free game (FreeGish's own pictures, levels and sounds), not Gish's
commercial data. For AmigaOS 3.x on a 68040 or 68060 with an FPU and an RTG
screen, on OpenGPU's SDL 2 and OpenGL.

Base: freegish_2.0.0+dfsg.orig.tar.xz from Debian (sha256
7a33acf869d9377cf904937f57b7cd2d55264672c02839074c0e1173d4582570):
upstream's 2.0.0 without its music, whose licence Debian could not
confirm. Debian's patches (2.0.0+dfsg-1) are the second commit. Branch
team2/freegish-amiga.

Status: plays in the lab (Instance-45 on GFOpenUp 0.8.19, 6 November
2026), slowly: the title screen, a new player, joining with the keyboard,
Story mode, a new game on Easy through the story pages, level 1-1 (Gish
lit and rolling in the sewer), the game's menu and Exit Game, the story
menu with Continue, and Exit back to the Shell; the settings, scores and
player written in user/.freegish. In the lab the game ran at about a fifth
of its speed (the clock took ten seconds a second), and keys and clicks
have to be held a moment to be seen. The lab is muted, so the sounds were
not heard there.


What you need
-------------

- A 68040 or 68060 with an FPU, AmigaOS 3.x, an RTG card (or the
  AmigaChrome lab), and a fast one: the game's physics and OpenGL drawing
  are heavy.
- GFOpenUp 0.8.18 or later: LIBS:OpenGPU/SDL2.module and GL.module.


How to play
-----------

Open the Freegish icon, or run Freegish from a Shell. Start, make a player
(type a name, Create), press Space to join with the keyboard, Continue,
then Story mode. Arrow keys roll Gish, Space jumps, A sticks, S slides, D
makes him heavy; Escape is the game's menu. The free game's datapack is in
the drawer's datapacks/; the settings, scores and players are kept in
user/.freegish.



Standard keys
-------------

Every AmiCradle game has the same keys on Right Amiga (GFOpenUp's
SDL2.module, after 0.8.26):

- Right Amiga + F: window or full screen, remembered for next time
- Right Amiga + P: pause (Right Amiga + P again goes on)
- Right Amiga + Q: quit
- Right Amiga + M: free the mouse pointer, or let the game take it again
- Right Amiga + S: a screenshot in the drawer's Screenshots
- Help: this game's keys (the drawer's Controls file)
- Right Amiga + F1, F2: F11, F12; Right Amiga + cursor keys: Page Up,
  Page Down, Home, End

The FULLSCREEN tooltype on the game's icon (take the brackets off) starts
it full screen.


Building
--------

    make -f amiga/Makefile.amigaos -j2 dist

with GFOpenAmigaGCC 16.2.0-open4, the OpenNDK headers, the OpenGPU kit and
the Team's static libpng and zlib (see the Makefile's head). The drawer is
dist/Freegish (and dist/Freegish.info).


What was changed
----------------

- Sound is OpenAL 1.1 from LIBS:GFOpenGPU/OpenAL.module (GFOpenUp
  installs it), shared by every program and played through AHI, as on any
  other system: the game links -lopenal. Without the module the game runs
  silent. libvorbisfile and libogg are stubs (amiga/include): there is no
  music to play.
- src/main.c: main() first moves into the drawer (amiga/amiga_start.c),
  where the game finds datapacks/. $HOME is the drawer's user/
  (amiga/amiga_env.c, -Wl,--wrap=getenv, from the GNU Robbo port), and
  chdir("..") is AmigaDOS's "/" (amiga/amiga_chdir.c, -Wl,--wrap=chdir,
  from the Holotz's Castle port).
- amiga/Makefile.amigaos builds src/CMakeLists.txt's sources with LINUX
  defined for the game's Unix code. amiga/stack.c: 1 MB of stack.
- As upstream ships the free datapack, its story pictures are placeholders
  ("story 800x600"), gishhurt.wav is missing and necksnap.wav is not a WAV
  file; the picture above level 1-1's Start showed as a magenta square in
  the lab.


Licences
--------

Freegish: GPL 2 or later (COPYING.txt). Its pictures, levels, animations
and sounds: CC BY-SA 3.0 unless noted (License-data.txt and the notes in
datapacks/). The Amiga files (amiga/): GPL 2 or later, the AmiCradle Team.
