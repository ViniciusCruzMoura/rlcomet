
OBJECTS = build/main.o \
	build/entity.o \
	build/sprite.o \
	build/camera.o

CC = gcc
MAKE = make

CFLAGS = -std=c99 -Wall -Wno-missing-braces -Wunused-result -D_DEFAULT_SOURCE \
	-Wextra -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
    -s -O2 -D_DEFAULT_SOURCE -Wl,-rpath

INCLUDE_PATHS = -Ivendor/raylib/src

LDFLAGS = -Lvendor/raylib/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lc

all: executable

build/camera.o: src/camera.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/sprite.o: src/sprite.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/entity.o: src/entity.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/main.o: src/main.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

# build the executable
executable: $(OBJECTS) libraylib.a
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# build raylib vendor
libraylib.a: vendor/raylib/src
	$(MAKE) -C $< PLATFORM=PLATFORM_DESKTOP

.PHONY: clean clean_vendor vendor

clean:
	@rm -f $(OBJECTS)

clean_vendor:
	$(MAKE) -C vendor/raylib/src clean

vendor:
	if [ ! -d "vendor/raylib" ]; then git clone -b 5.5 --depth=1 https://github.com/raysan5/raylib.git vendor/raylib; fi
