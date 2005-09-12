CPPFLAGS=
LDFLAGS=
LIBS=-lm
CFLAGS=-Wall -W
IMAGE=image.o

# comment these out if you don't want ImageMagick support
#CPPFLAGS+= `Wand-config --cppflags`
#LDFLAGS+= `Wand-config --ldflags`
#LIBS+= `Wand-config --libs`
#CFLAGS+= `Wand-config --cflags`
#IMAGE=magickimage.o

CC=gcc

all: codetest imagetest codegen v4ltest

codetest: codetest.o recognise.o ${IMAGE} crc.o codegen_util.o
	$(CC) ${CFLAGS} -o $@ $^ ${LDFLAGS} ${LIBS}

imagetest: imagetest.o recognise.o ${IMAGE} crc.o codegen_util.o
	$(CC) ${CFLAGS} -o $@ $^ ${LDFLAGS} ${LIBS}

codegen: codegen.o recognise.o ${IMAGE} crc.o codegen_util.o
	$(CC) ${CFLAGS} -o $@ $^ ${LDFLAGS} ${LIBS}

v4ltest: v4ltest.o recognise.o v4l.o codegen_util.o crc.o ${IMAGE}
	$(CC) ${CFLAGS} -o $@ $^ ${LDFLAGS} ${LIBS}

%.o: %.c
	$(CC) ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

%.o: Makefile

polydef.h: poly.pl
	perl poly.pl > polydef.h

clean: 
	-rm -f *.o codetest imagetest codegen recognise crc polydef.h v4ltest

camera.o: camera.h
codegen_util.o: options.h codegen_util.h
codetest.o: options.h image.h recognise.h
framebuffer.o: util.h options.h recognise.h
image.o: image.h options.h
imagetest.o: options.h image.h recognise.h
magickimage.o: image.h options.h
recognise.o: image.h options.h recognise.h
crc.o: options.h crc.h polydef.h
v4l.o: v4l.h options.h