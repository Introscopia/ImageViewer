CC = gcc

OBJS = main.c basics.c
	
ifeq ($(OS),Windows_NT) # Windows_NT is the identifier for all versions of Windows
	DETECTED_OS := Windows
else
	DETECTED_OS := $(shell uname)
endif

ifeq ($(DETECTED_OS),Windows)
	INCLUDE_PATHS = -IC:/SDL/SDL2-2.0.10/i686-w64-mingw32/include/SDL2
	INCLUDE_PATHS += -IC:/SDL/SDL2_image-2.0.5/i686-w64-mingw32/include/SDL2
	LIBRARY_PATHS = -LC:/SDL/SDL2-2.0.10/i686-w64-mingw32/lib
	LIBRARY_PATHS += -LC:/SDL/SDL2_image-2.0.5/i686-w64-mingw32/lib

	LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
else
	INCLUDE_PATHS = -I/usr/include/SDL2
	LINKER_FLAGS = -lm -lSDL2 -lSDL2_image
endif


# -w (suppresses all warnings)
# -Wl,-subsystem (windows gets rid of the console window)
COMPILER_FLAGS = -w -Wl,-subsystem,windows
# COMPILER_FLAGS = -Wall -Wextra -Werror -O2 -std=c99 -pedantic
# COMPILER_FLAGS = -w
# COMPILER_FLAGS = -Wall -g

OBJ_NAME = ImageViewer

all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)