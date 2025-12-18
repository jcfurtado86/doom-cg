# DoomLike Makefile
# - Compila .o em build/
# - Linka o executável em build/DoomLike
# - Suporta organização em src/ e headers em include/

CXX       := g++
CXXFLAGS  := -g -O0 -Wall -Wextra -Iinclude
LDFLAGS   :=
LDLIBS    := -lGLEW -lGL -lGLU -lglut

TARGET    := DoomLike
BUILD_DIR := build

SRC_DIR   := src
MAIN      := main.cpp

# Encontra todos os .cpp dentro de src (recursivo)
SRCS      := $(shell find $(SRC_DIR) -name '*.cpp' | sort)
SRCS      += $(MAIN)

# Converte paths em nomes de objetos dentro de build/
# Ex:
#   src/core/app.cpp -> build/src/core/app.o
#   main.cpp         -> build/main.o
OBJS      := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run dirs

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): dirs $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

# Regra genérica: compila qualquer .cpp para build/.../.o
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

dirs:
	@mkdir -p $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
