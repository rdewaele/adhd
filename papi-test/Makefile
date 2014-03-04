PROGRAM = papi-test
SOURCES = main.cpp
LDLIBS = -lpapi

CXXFLAGS := -std=c++11 $(CXXFLAGS)

PAPI_INSTALL_DIR = install

OBJECTS = $(SOURCES:.cpp=.o)

all: $(PROGRAM)

static: LDFLAGS = -static -L$(PAPI_INSTALL_DIR)/lib
static: CXXFLAGS := -I$(PAPI_INSTALL_DIR)/include $(CXXFLAGS)

static: all

run: $(PROGRAM)
	./$<

$(PROGRAM): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

analyze:
	clang $(CXXFLAGS) --analyze $(SOURCES)


PAPI_DIR = papi-5.3.0

papi-lib: | $(PAPI_DIR)
		cd $(PAPI_DIR)/src \
		&& export I_MPI_CC=icc \
		; ./configure --prefix=$(PWD)/$(PAPI_INSTALL_DIR) --with-static-lib=yes --with-shared-lib=yes --with-perf-events \
		&& make \
		&& make test \
		&& make install

$(PAPI_DIR):
	curl http://icl.cs.utk.edu/projects/papi/downloads/papi-5.3.0.tar.gz | tar xz

clean:
	rm -rf $(PROGRAM) $(OBJECTS) $(MAKEDEP) $(wildcard *.plist) $(PAPI_DIR)

$(MAKEDEP): $(SOURCES)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

.PHONY: all clean analyze run papi-lib

include $(MAKEDEP)
