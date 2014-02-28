PROGRAM = papi-test
SOURCES = main.cpp
LDLIBS = -lpapi

CXXFLAGS := -std=c++11 $(CXXFLAGS)

OBJECTS = $(SOURCES:.cpp=.o)

all: $(PROGRAM)

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
		&& ./configure --prefix=$(PWD)/install --with-static-lib=yes --with-shared-lib=yes --with-perf-events \
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
