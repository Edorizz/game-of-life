.POSIX:
.SUFFIX:

CC := gcc
CFLAGS := -c -std=c99 -pedantic -Wall
LDLIBS := -ldmatrix -lGL -lGLU -lGLEW -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl -lXinerama -lXcursor -lm
RM := rm -f
MKDIR := mkdir -p
NAME := life

all: obj/ $(NAME)

obj/:
	$(MKDIR) obj/

$(NAME): obj/life.o
	$(CC) $^ $(LDLIBS) -o $@

obj/life.o: src/life.c
	$(CC) $(CFLAGS) $^ -o $@

.PHONY:
clean:
	$(RM) obj/*.o $(NAME)

