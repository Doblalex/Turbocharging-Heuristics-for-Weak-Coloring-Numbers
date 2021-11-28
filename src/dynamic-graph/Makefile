OPTFLAGS=-O2 -std=c++17
OPTLOCALFLAGS= -DLOCAL -std=c++17 -O2
CXXFLAGS=-DLOCAL -std=c++17 -fPIC
#LDFLAGS += -fsanitize=address -fsanitize=undefined

# Boost C++ library
ROOT_BOOST = /usr/lib/x86_64-linux-gnu/

LDFLAGS += -L$(ROOT_BOOST) -lboost_stacktrace
SOURCES=$(wildcard *.cpp)
DEPENDS=$(patsubst %.cpp,.%.depends,$(SOURCES))
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))
	
.%.depends: %.cpp
	$(CXX) $(OPTFLAGS) -MM $< -o $@ $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f *.o *.so $(EXECS) $(DEPENDS)

libdynamic-graph.so: assert.o dynamic_connectivity.o dynamic_forest.o graph.o hash.o sequence.o
	gcc -shared $^ -o $@

all: assert.o dynamic_connectivity.o dynamic_forest.o graph.o hash.o sequence.o libdynamic-graph.so

-include $(DEPENDS)