
SOURCES = \
	src/main.c \
	src/entity.c \
	src/sprite.c \
	src/console.c \
	src/assets.c \
	src/level.c \
	src/level_editor.c \
	src/single_header_impl.c \
	src/camera.c

OBJECTS = $(SOURCES:src/%.c=build/%.o)
MPACK_DIR = thirdparty/mpack/src/mpack

CC = gcc
MAKE = make

CFLAGS = -std=c99 -Wall -Wno-missing-braces -Wunused-result -D_DEFAULT_SOURCE \
	-Wextra -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
    -s -O2 -D_DEFAULT_SOURCE -MMD -MP

INCLUDE_PATHS = -Ithirdparty/raylib/src -Ithirdparty -Ithirdparty/mpack/src -Iinclude

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

build/level.o: thirdparty/mpack/src/mpack/mpack.h

build/libmpack.a: thirdparty/mpack/src/mpack/mpack.h $(wildcard $(MPACK_DIR)/*.c)
	@mkdir -p build/mpack
	@for src in $(MPACK_DIR)/*.c; do \
		$(CC) $(CFLAGS) $(INCLUDE_PATHS) -c "$$src" -o "build/mpack/$$(basename "$$src" .c).o" || exit 1; \
	done
	ar rcs $@ build/mpack/*.o

executable$(EXE): $(OBJECTS) build/libmpack.a thirdparty/raylib/src/libraylib.a
	$(CC) $(OBJECTS) build/libmpack.a -o $@ $(LDFLAGS)

thirdparty/mpack/src/mpack/mpack.h:
	git clone --branch v1.1.1 --depth 1 https://github.com/ludocode/mpack.git thirdparty/mpack

thirdparty/raylib/src/libraylib.a: thirdparty/raylib/src
	$(MAKE) -C $< PLATFORM=PLATFORM_DESKTOP

thirdparty/raylib/src/Makefile:
	git clone --branch 6.2 --depth 1 https://github.com/raysan5/raylib.git thirdparty/raylib

thirdparty: thirdparty/raylib/src/Makefile thirdparty/mpack/src/mpack/mpack.h

clean:
	@rm -rf build/

clean_all:
	@rm -rf build/
	@rm -rf thirdparty/raylib
	@rm -rf thirdparty/mpack
	@rm -f executable$(EXE)


-include $(OBJECTS:.o=.d)
