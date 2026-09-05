CC      ?= gcc
CFLAGS  ?= -std=c17 -g\
	-D_POSIX_SOURCE -D_DEFAULT_SOURCE\
	-Wall -Werror -pedantic\
	-I. -I../../vendor/SDL2/include/SDL2 \

LIBOBJS = display/display.o \
		  dataExtraction/dataExtraction.o \
		  fft/fft.o\
		  barLogic/barLogic.o \



LIB = libextension.a \

all : $(LIB)

$(LIB) : $(LIBOBJS)
	ar rcs $(LIB) $(LIBOBJS)

clean:
	rm -f $(LIBOBJS) $(LIB)