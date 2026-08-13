CXX ?= c++
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?= $(shell pkg-config --cflags glfw3 vulkan)
LDLIBS ?= $(shell pkg-config --libs glfw3 vulkan)

TARGET := EVO
SOURCES := $(shell find src -name '*.cpp' -print)
CPPFLAGS += -Isrc

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)
