
SOURCES = \
	src/main.c \
	src/entity.c \
	src/sprite.c \
	src/console.c \
	src/stb_ds.c \
	src/pmem_usage.c \
	src/camera.c

OBJECTS = $(SOURCES:src/%.c=build/%.o)

CC = gcc
MAKE = make

CFLAGS = -std=c99 -Wall -Wno-missing-braces -Wunused-result -D_DEFAULT_SOURCE \
	-Wextra -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
    -s -O2 -D_DEFAULT_SOURCE -Wl,-rpath

INCLUDE_PATHS = -Ithirdparty/raylib/src

LDFLAGS = -Lthirdparty/raylib/src -lraylib
ifeq ($(OS),Windows_NT)
	LDFLAGS += -lopengl32 -lgdi32 -lwinmm
else
	LDFLAGS += -lGL -lm -lpthread -ldl -lrt -lX11 -lc
endif

EXE := 
ifeq ($(OS),Windows_NT)
	EXE := .exe
endif

.PHONY: clean thirdparty
all: executable$(EXE)

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

executable$(EXE): $(OBJECTS) thirdparty/raylib/src/libraylib.a
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

thirdparty/raylib/src/libraylib.a: thirdparty/raylib/src
	$(MAKE) -C $< PLATFORM=PLATFORM_DESKTOP

thirdparty/raylib/src/Makefile:
	git clone --branch 5.5 --depth 1 https://github.com/raysan5/raylib.git thirdparty/raylib

thirdparty: thirdparty/raylib/src/Makefile

clean:
	@rm $(OBJECTS)

clean_all:
	@rm -rf build/
	@rm -rf thirdparty/
	@rm -f executable$(EXE)

