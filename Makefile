leanCC      ?= gcc
CFLAGS  ?= -std=c17 -g\
	-D_POSIX_SOURCE -D_DEFAULT_SOURCE\
	-Wall -Werror -pedantic \
	-IextensionSrc \
	-I../vendor/SDL2/include/SDL2\

.SUFFIXES: .c .o

.PHONY: all clean
EXT_DIR = extensionSrc
EXT_LIB = $(EXT_DIR)/libextension.a

all : visualizer

$(EXT_LIB):
	$(MAKE) -C $(EXT_DIR)

visualizer : visualizer.c $(EXT_LIB)
	$(CC) $(CFLAGS) visualizer.c -L$(EXT_DIR) -lextension -L../vendor/SDL2/lib -lSDL2 -lm -o visualizer

clean:
	rm -f *.o visualizer emulate 
	$(MAKE) -C $(EXT_DIR) clean