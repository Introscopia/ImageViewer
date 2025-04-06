CC = gcc

OBJS = imgview.c 
	
ifeq ($(OS),Windows_NT) # Windows_NT is the identifier for all versions of Windows
	DETECTED_OS := Windows
else
	DETECTED_OS := $(shell uname)
endif

ifeq ($(DETECTED_OS),Windows)
	INCLUDE_PATHS = -IC:/SDL/SDL3-3.2.10/x86_64-w64-mingw32/include/SDL3
	INCLUDE_PATHS += -IC:/SDL/SDL3-3.2.10/x86_64-w64-mingw32/include/
	INCLUDE_PATHS += -IC:/SDL/SDL3_image-3.2.4/x86_64-w64-mingw32/include/SDL3_image

	LIBRARY_PATHS = -LC:/SDL/SDL3-3.2.10/x86_64-w64-mingw32/lib
	LIBRARY_PATHS += -LC:/SDL/SDL3_image-3.2.4/x86_64-w64-mingw32/lib

	LINKER_FLAGS = -lmingw32 -lSDL3 -lSDL3_image
else
	INCLUDE_PATHS = -I/usr/include/SDL3
	LINKER_FLAGS = -lm -lSDL3 -lSDL3_image
endif


COMPILER_FLAGS_RELEASE = -w -Wl,-subsystem,windows
COMPILER_FLAGS_QUICK = -w
COMPILER_FLAGS_DEBUG = -fmax-errors=3 -Waddress -Warray-bounds=1 -Wbool-compare -Wformat -Wimplicit -Wlogical-not-parentheses -Wmaybe-uninitialized -Wmemset-elt-size -Wmemset-transposed-args -Wmissing-braces -Wmultistatement-macros -Wopenmp-simd -Wparentheses -Wpointer-sign -Wrestrict -Wreturn-type -Wsequence-point -Wsizeof-pointer-div -Wsizeof-pointer-memaccess -Wstrict-aliasing -Wstrict-overflow=1 -Wtautological-compare -Wtrigraphs -Wuninitialized -Wunknown-pragmas -Wvolatile-register-var -Wcast-function-type -Wmissing-field-initializers -Wmissing-parameter-type -Woverride-init -Wsign-compare -Wtype-limits -Wshift-negative-value
COMPILER_FLAGS_MAX = -Wall -Wextra -Werror -O2 -std=c99 -pedantic

OBJ_NAME = ImageViewer

release : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS_RELEASE) $(LINKER_FLAGS) -std=c11 -o $(OBJ_NAME)
quick : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS_QUICK) $(LINKER_FLAGS) -std=c11 -o $(OBJ_NAME)
debug : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS_DEBUG) $(LINKER_FLAGS) -std=c11 -o $(OBJ_NAME)
max : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS_MAX) $(LINKER_FLAGS) -std=c11 -o $(OBJ_NAME)