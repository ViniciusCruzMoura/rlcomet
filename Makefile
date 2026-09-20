
OBJECTS = build/main.o \
	build/entity.o \
	build/sprite.o \
	build/console.o \
	build/darray.o \
	build/camera.o

CC = gcc
MAKE = make

CFLAGS = -std=c99 -Wall -Wno-missing-braces -Wunused-result -D_DEFAULT_SOURCE \
	-Wextra -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
    -s -O2 -D_DEFAULT_SOURCE -Wl,-rpath

INCLUDE_PATHS = -Ithirdparty/raylib/src

LDFLAGS = -Lthirdparty/raylib/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lc

all: build/ executable

build/:
	@mkdir -p build/

build/darray.o: src/darray.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/console.o: src/console.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/camera.o: src/camera.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/sprite.o: src/sprite.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/entity.o: src/entity.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

build/main.o: src/main.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

executable: $(OBJECTS) libraylib.a
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

libraylib.a: thirdparty/raylib/src
	$(MAKE) -C $< PLATFORM=PLATFORM_DESKTOP

.PHONY: clean clean_thirdparty thirdparty

clean:
	@rm $(OBJECTS)

clean_thirdparty:
	$(MAKE) -C thirdparty/raylib/src clean

clean_all:
	@rm -rf build/
	@rm -rf thirdparty/
	@rm executable

thirdparty:
	if [ ! -d "thirdparty/raylib" ]; then git clone -b 5.5 --depth=1 https://github.com/raysan5/raylib.git thirdparty/raylib; fi
