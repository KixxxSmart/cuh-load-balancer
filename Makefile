CXX = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -MMD -MP
CPPFLAGS = -Iinclude

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.cpp,build/%.o,$(SOURCES))

.PHONY: all docs clean
all: build/cuh

build/cuh: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

docs:
	doxygen Doxyfile

clean:
	rm -rf build

-include $(OBJECTS:.o=.d)
