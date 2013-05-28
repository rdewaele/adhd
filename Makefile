# main executable filename
PROGRAM = adhd

# dependencies supporting pkg-config
PKGCONFIG_LIBS = libconfig

# One could play with compiler optimizations to see whether those have any
# effect.
EXTRA_WARNINGS := -Wconversion -Wshadow -Wpointer-arith -Wcast-qual \
								 -Wwrite-strings
# conditionally add warnings known not to be recongnised by icc
ifneq ($(CC),icc)
	EXTRA_WARNINGS += -Wcast-align
endif
CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -W -Wall -Wextra -pedantic \
	$(EXTRA_WARNINGS) \
	-O3 -funroll-loops \
	-DNDEBUG \
	-pthread \
	$(shell pkg-config --cflags $(PKGCONFIG_LIBS)) \
	$(CFLAGS)
LDFLAGS += -lm -lrt \
					 $(shell pkg-config --libs $(PKGCONFIG_LIBS))

SOURCES = main.c arraywalk.c util.c logging.c options.c parallel.c
VALGRIND_CONF = vgconfig.cfg
OBJECTS = $(SOURCES:.c=.o)

MAKEDEP = .make.dep

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# also depend on Makefile because we like to fiddle with it ;-)
%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROGRAM) $(OBJECTS) $(MAKEDEP) $(SOURCES:.c=.plist)

analyze:
	clang --analyze $(SOURCES)

valgrind: $(PROGRAM) $(VALGRIND_CONF)
	valgrind -v --leak-check=full ./$< -i -c $(VALGRIND_CONF)

.PHONY: clean analyze

$(MAKEDEP):
	$(CC) -MM $(CFLAGS) $(SOURCES) > $@

include $(MAKEDEP)
