SOURCES = main.cpp

CXXFLAGS := -std=c++11 -O3 -I$(PAPI_HOST)/include -I$(PAPI_MIC)/include $(CXXFLAGS)
LDFLAGS := -L$(PAPI_HOST)/lib -L$(PAPI_MIC)/lib $(LDFLAGS)

LDLIBS = -Bstatic -lpapi

ifndef $(CXX)
CXX=icpc
endif

test-host: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test-mic: $(SOURCES)
	icpc -mmic $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

run-host: test-host
	./$<

run-mic: test-mic
	micnativeloadex $<

clean:
	rm -rf test-host test-mic $(MAKEDEP) $(wildcard *.plist)

$(MAKEDEP): $(SOURCES)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

.PHONY: all clean run

include $(MAKEDEP)
