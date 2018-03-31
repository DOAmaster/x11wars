
# for sound, add the following libraries to the build.
# /usr/lib/x86_64-linux-gnu/libopenal.so /usr/lib/libalut.so
#
# like this:
#	g++ bump.cpp libggfonts.a -Wall -obump -lX11 -lGL -lGLU -lm -lrt \
#	/usr/lib/x86_64-linux-gnu/libopenal.so \
#	/usr/lib/libalut.so
#

all: x11wars

x11wars: x11wars.cpp fonts.h
	g++ x11wars.cpp libggfonts.a -Wall -ox11wars -lX11 -lGL -lGLU -lm -lrt

clean:
	rm -f x11wars
	rm -f *.o

