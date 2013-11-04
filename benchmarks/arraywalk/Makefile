LIBRARY = libarraywalk.a
CXXPROGRAM = arraywalk_cpp
CCPROGRAM = arraywalk_c

all: $(CXXPROGRAM) $(CCPROGRAM)

LIBSOURCES = arraywalk.cpp c_bindings.cpp
CXXSOURCES = cpp_main.cpp
CCSOURCES = c_main.c

LIBOBJECTS = $(LIBSOURCES:.cpp=.o)
CXXOBJECTS = $(CXXSOURCES:.cpp=.o)
CCOBJECTS = $(CCSOURCES:.c=.o)

MAKEDEP = .make.dep
# One could play with compiler optimizations to see whether those have any
# effect.
EXTRA_WARNINGS := -Wconversion -Wshadow -Wpointer-arith -Wcast-qual \
								 -Wwrite-strings -Wunused
# warnings unrecognised by icc
ifneq ($(CXX),icpc)
	EXTRA_WARNINGS += -Wcast-align
	CXXFLAGS += -march=native -mtune=native
endif
# make icc report very elaborately about vectorization successes and failures
ifeq ($(CXX),icpc)
	CXXFLAGS += -xHost
endif
CFLAGS := -std=c99 -W -Wall -Wextra -pedantic $(CFLAGS) \
	-O3

CXXFLAGS := -std=c++11 -W -Wall -Wextra -pedantic \
	$(EXTRA_WARNINGS) \
	$(CXXFLAGS) \
	-g -O3
#	-DNDEBUG

LDLIBS += -larraywalk -lm -lrt -lstdc++
LDFLAGS += -L.

test: $(PROGRAM)
	./$<

run: test

$(CXXPROGRAM): $(LIBRARY) $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CXXOBJECTS) $(LDLIBS) -o $@

$(CCPROGRAM): $(LIBRARY) $(CCOBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CCOBJECTS) $(LDLIBS) -o $@

$(LIBRARY): $(LIBOBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(CXXOBJECTS:%.o):%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CCOBJECTS:%.o):%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CXXPROGRAM) $(CCPROGRAM) $(LIBRARY) $(CXXOBJECTS) $(CCOBJECTS) \
		$(LIBOBJECTS) $(MAKEDEP) $(wildcard *.plist)

analyze:
	clang $(CXXFLAGS) --analyze $(SOURCES) $(LIBSOURCES)

valgrind: $(PROGRAMS) $(VALGRIND_CONF)
	valgrind -v --leak-check=full --show-reachable=yes ./$< -i -c $(VALGRIND_CONF)

$(MAKEDEP): $(CXXSOURCES) $(CCSOURCES)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

.PHONY: all clean analyze test run

include $(MAKEDEP)
