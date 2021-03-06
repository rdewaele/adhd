LIBRARY = libreduction.a
PROGRAM = reduction

all: $(PROGRAM)

LIBSOURCES = reduction.cpp config.cpp timings.cpp
SOURCES = main.cpp

LIBOBJECTS = $(LIBSOURCES:.cpp=.o)
OBJECTS = $(SOURCES:.cpp=.o)

MAKEDEP = .make.dep
# One could play with compiler optimizations to see whether those have any
# effect.
EXTRA_WARNINGS := -Wconversion -Wshadow -Wpointer-arith -Wcast-qual \
								 -Wwrite-strings -Wunused
# warnings unrecognised by icc
ifneq ($(CXX),icpc)
	EXTRA_WARNINGS += -Wcast-align
	CXXFLAGS := -march=native -mtune=native $(CXXFLAGS)
endif
# make icc report very elaborately about vectorization successes and failures
ifeq ($(CXX),icpc)
	CXXFLAGS += -xHost
endif

CXXFLAGS := -std=c++11 -W -Wall -Wextra -pedantic \
	$(EXTRA_WARNINGS) \
	-g -O3 \
	$(CXXFLAGS)

LDLIBS += -lreduction -lbenchmark -lm -lrt -lstdc++
LDFLAGS += -L. -L..

test: $(PROGRAM)
	./$<

run: test

$(PROGRAM): $(LIBRARY) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(LIBRARY): $(LIBOBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(OBJECTS:%.o):%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(PROGRAM) $(LIBRARY) $(OBJECTS) \
		$(LIBOBJECTS) $(MAKEDEP) $(wildcard *.plist)

analyze:
	clang $(CXXFLAGS) --analyze $(SOURCES) $(LIBSOURCES)

valgrind: $(PROGRAM)
	valgrind -v --leak-check=full --show-reachable=yes ./$<

$(MAKEDEP): $(SOURCES) $(LIBSOURCES)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

.PHONY: all clean analyze test run

include $(MAKEDEP)
