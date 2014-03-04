PROGRAM = papi-test
SOURCES = main.cpp
LDLIBS = -lpapi

CXXFLAGS := -std=c++11 $(CXXFLAGS)
ifdef PAPI_INSTALL_DIR
	CXXFLAGS := -I$(PAPI_INSTALL_DIR)/include $(CXXFLAGS)
	LDFLAGS := -L$(PAPI_INSTALL_DIR)/lib
endif

OBJECTS = $(SOURCES:.cpp=.o)


# $(PROGRAM) related targets

all: $(PROGRAM)

run: $(PROGRAM)
	./$<

$(PROGRAM): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

%.o: %.cpp | $(PAPI_INSTALL_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

analyze:
	clang $(CXXFLAGS) --analyze $(SOURCES)


# PAPI specifics

PAPI_DIR = papi-5.3.0
PAPI_TAR = $(PAPI_DIR).tar.gz
PAPI_SRC = http://icl.cs.utk.edu/projects/papi/downloads/$(PAPI_TAR)

$(PAPI_INSTALL_DIR): | $(PAPI_DIR)
		cd $(PAPI_DIR)/src \
		&& export I_MPI_CC=icc \
		; ./configure --prefix=$(PWD)/$(PAPI_INSTALL_DIR) --with-static-lib=yes --with-shared-lib=yes --with-perf-events \
		&& $(MAKE) \
		&& $(MAKE) test \
		&& $(MAKE) install

$(PAPI_DIR): | $(PAPI_TAR)
	tar xf $(PAPI_TAR)

$(PAPI_TAR):
	curl $(PAPI_SRC) -o $(PAPI_TAR)


# misc. targets

clean:
	rm -rf $(PROGRAM) $(OBJECTS) $(MAKEDEP) $(wildcard *.plist)

$(MAKEDEP): $(SOURCES)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

.PHONY: all clean analyze run papi-lib

include $(MAKEDEP)
