CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -O2 -DGL_SILENCE_DEPRECATION
LDFLAGS = -pthread -lm -framework GLUT -framework OpenGL

SOURCES = main.c \
          config.c \
          maze.c \
          family.c \
          simulation.c \
          utils.c \
          female_ape.c \
          male_ape.c \
          baby_ape.c \
          graphics.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = ape_simulation

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	gdb ./$(TARGET)

.PHONY: all clean run debug
