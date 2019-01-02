SOURCES = main.cpp
HEADERS = 
CFLAGS = -Wall -std=c++14 -g `sdl2-config --cflags`
LFLAGS = `sdl2-config --libs` -lGL
OUT = mgs_playground

$(OUT): $(SOURCES) $(HEADERS)
	g++ -o $@ $(CFLAGS) $(SOURCES) $(LFLAGS)