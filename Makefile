CC = g++
CXXFLAGS = -std=c++11 -O2 -g
LIBS = -lGLEW -lGL -lGLU -lglut

SRCS = main.cpp draw.cpp input.cpp scene.cpp texture.cpp shader.cpp transform.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = DoomLike

.PHONY: all run clean rebuild

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

rebuild: clean all

clean:
	rm -f $(OBJS) $(TARGET)
