SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*.hpp)
CFLAGS = -Wall -std=c++14 -g `sdl2-config --cflags`
LFLAGS = `sdl2-config --libs` -lGL
OUT = mgs_playground

$(OUT): $(SOURCES) $(HEADERS)
	g++ -o $@ $(CFLAGS) $(SOURCES) $(LFLAGS)

.PHONY: run
run: $(OUT)
	./$(OUT)