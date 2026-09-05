CC ?= gcc

CFLAGS  ?= -std=c17 -g\
	-D_POSIX_SOURCE -D_DEFAULT_SOURCE\
	-Wall -Werror -pedantic \
	-I../extensionSrc \
	-I../vendor/SDL2/include/SDL2

EXT_DIR = ../extensionSrc
EXT_LIB = $(EXT_DIR)/libextension.a

#Gets all c files starting with test_
TEST_SRCS = $(wildcard test_*.c)

# Strip the ".c" from the file names to get the target executable names
TEST_BINS = $(TEST_SRCS:.c=)

all: $(TEST_BINS)

.SUFFIXES: .c .o

.PHONY: all clean

$(EXT_LIB):
	$(MAKE) -C $(EXT_DIR)

#Whatever is made, it is representted by the percentage sign
#It looks for a file that is %.c and ensures the emulator is up to date
%:%.c $(EXT_LIB)
	$(CC) $(CFLAGS) $< $(EXT_LIB) -o $@ -lm

cleanTest:
	rm -f $(TEST_BINS) *.o

clean:
	rm -f $(TEST_BINS) *.o
	$(MAKE) -C $(EXT_DIR) clean