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

clean:
	rm -f $(PROGRAM) $(OBJECTS) $(MAKEDEP) $(wildcard *.plist)

analyze:
	clang $(CXXFLAGS) --analyze $(SOURCES)

$(MAKEDEP): $(SOURCES)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

.PHONY: all clean analyze run

include $(MAKEDEP)
