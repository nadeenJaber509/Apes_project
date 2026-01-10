# -------- Project: Apes Simulation --------

CC      = gcc
TARGET  = ape_simulation

SRC = main.c config.c maze.c family.c simulation.c utils.c female_ape.c male_ape.c baby_ape.c graphics.c
OBJ = $(SRC:.c=.o)

CFLAGS  = -Wall -Wextra -pthread -g -O2 -DGL_SILENCE_DEPRECATION
LDFLAGS = -pthread

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    LIBS = -lm -framework GLUT -framework OpenGL
else
    LIBS = -lm -lglut -lGL -lGLU
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS) $(LIBS)

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET) config.txt

.PHONY: all clean run
