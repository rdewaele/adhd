# main executable filename
PROGRAM = adhd

# dependencies supporting pkg-config
PKGCONFIG_LIBS = libconfig

# One could play with compiler optimizations to see whether those have any
# effect.
EXTRA_WARNINGS := -Wconversion -Wshadow -Wpointer-arith -Wcast-qual \
								 -Wwrite-strings
# conditionally add warnings known not to be recongnised by icc
ifneq ($(CXX),icc)
	EXTRA_WARNINGS += -Wcast-align
endif
# make icc report very elaborately about vectorization successes and failures
ifeq ($(CXX),icc)
	CXXFLAGS += -vec-report=6
endif
CXXFLAGS := -std=c++11 -D_POSIX_C_SOURCE=200809L -W -Wall -Wextra -pedantic \
	$(EXTRA_WARNINGS) \
	-O3 -g -funroll-loops \
	-pthread \
	$(shell pkg-config --cflags $(PKGCONFIG_LIBS)) \
	$(CXXFLAGS) \
	-DNDEBUG
LDFLAGS += -lm -lrt \
					 $(shell pkg-config --libs $(PKGCONFIG_LIBS))

SOURCES = main.c logging.c options.c parallel.c \
					benchmarks/util.c \
					benchmarks/arraywalk.c \
					benchmarks/flops.c \
					benchmarks/streaming.c
VALGRIND_CONF = vgconfig.cfg
OBJECTS = $(SOURCES:.c=.o)

MAKEDEP = .make.dep

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# also depend on Makefile because we like to fiddle with it ;-)
%.o: %.c Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(PROGRAM) $(OBJECTS) $(MAKEDEP) $(SOURCES:.c=.plist)

analyze:
	clang --analyze $(SOURCES)

valgrind: $(PROGRAM) $(VALGRIND_CONF)
	valgrind -v --leak-check=full --show-reachable=yes ./$< -i -c $(VALGRIND_CONF)

.PHONY: clean analyze

$(MAKEDEP):
	$(CXX) -MM $(CXXFLAGS) $(SOURCES) > $@

include $(MAKEDEP)
