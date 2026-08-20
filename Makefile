CXX ?= c++
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
CPPFLAGS ?= $(shell pkg-config --cflags glfw3 vulkan)
LDLIBS ?= $(shell pkg-config --libs glfw3 vulkan gl)

TARGET := EVO
SOURCES := $(shell find src -name '*.cpp' -print)
SHADERS := shaders/land.vert.spv shaders/land.frag.spv
CPPFLAGS += -Isrc

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(SHADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@ $(LDLIBS)

shaders/%.vert.spv: shaders/%.vert
	glslc $< -o $@

shaders/%.frag.spv: shaders/%.frag
	glslc $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) $(SHADERS)
