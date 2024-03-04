Here it is! SRB2 Final Demo 1.09 (in development) source code!


Win32 with Visual C 6/7
~~~

2 VC++ 6.0 project files are included:

Win32/DirectX/FMOD
src\win32\wLegacy.dsw
You'll need FMOD to compile this version (www.fmod.org)
or
Win32/SDL/SDL_mixer
src\sdl\Win32SDL.dsp
You'll need SDL and SDL_mixer for this version (www.libsdl.org)

Both needs NASM

No warranty, support, etc. of any kind is offered,
just plain old as is.
Some bits of code are still really scary.
If you're planning a big project based off of this,
use 1.09 to learn what you're doing, then
base your game off of the forthcoming 1.10 source.
Go nuts!


Win32 with Dev-C++ (http://bloodshed.net/ free!)
~~~
2 Dev-C++ project files are included:

Win32/DirectX/FMOD
src\win32\SRB2.dev
or
Win32/SDL/SDL_mixer
src\sdl\Win32SDL.dev
You'll need SDL and SDL_mixer for this version (www.libsdl.org)

you will also need NASM and DirectX 6 (or up) Dev-Paks for the Dev-C++ projects

GNU/Linux
~~~

Dependencies:
  SDL 1.2.7 or better (from libsdl.org)
  SDL_Mixer 1.5 (from libsdl.org)
  Nasm (use NOASM=1 if you don't have it or have an non-i386 system, I think)
  The Xiph.org libogg and libvorbis libraries
  The OpenGL headers (from Mesa, usually shipped with your X.org or XFree
    installation, so you needn't worry, most likely)
  GCC 3.x toolchain and binutils
  GNU Make

Build instructions:

cd src
make LINUX=1 # you may optionally add DEBUGMODE=1 to build it
             # with debugging symbols

DJGPP/DOS
~~~

Dependencies:
  Allegro 3.12 game programming library, (from 
  http://alleg.sourceforge.net/index.html)
  Nasm (use NOASM=1 if you don't have it)
  libsocket (from http://homepages.nildram.co.uk/~phekda/richdawe/lsck/) or
  Watt-32 (from http://www.bgnett.no/~giva/)
  GCC 3.x toolchain and binutils
  GNU Make

Build instructions:

cd src
make  # to link with Watt-32, add WATTCP=1
      # you may optionally add DEBUGMODE=1 to build it with debugging symbols
      # for remote debugging over the COM port, add RDB=1

Notes:
 use tools\djgpp\all313.diff to update Allegro to a "more usable" version ;)
 Example: E:\djgpp\allegro>patch -p# < D:\SRB2Code\1.09\srb2\tools\djgpp\all313.diff

Windows CE
~~~

Dependencies:
  SDL 1.27

Build instructions:

use src\SDL\WinCE\SRB2CE.vcw

-------------------------------------------------------------------------------

binaries will turn in up in bin/

note: read the src/makefile for more options

- Sonic Team Junior
http://www.srb2.org
