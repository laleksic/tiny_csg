CC = g++
CFLAGS = -fpic -c -Wall -std=c++11 -pedantic
LDFLAGS = -lGLEW -lGL

_OBJ = shader.o shader_program.o
OBJ = $(addprefix obj/,$(_OBJ))

# Include libs
LDFLAGS +=

all: libshader.a libshader.so libshader

obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ -Iinclude

libshader.a: $(OBJ) $(LDFLAGS)
	ar -rcs $@ $^

libshader.so: $(OBJ) $(LDFLAGS)
	$(CC) -shared -o $@ $^

libshader: include/shader.hpp include/shader_program.hpp\
           include/shader_error.hpp
	cat $^ > $@

install: libshader.a libshader.so libshader
	@mkdir -p "$$HOME/.local/lib" "$$HOME/.local/include"
	cp libshader.a libshader.so "$$HOME/.local/lib/"
	cp libshader "$$HOME/.local/include"

.PHONY: clean install

clean:
	rm libshader libshader.a libshader.so obj/*
