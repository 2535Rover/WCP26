SOURCES=main.cpp camera.cpp
HEADERS=camera.hpp
LFLAGS=`sdl2-config --libs` -L/opt/libjpeg-turbo/lib64 -l:libturbojpeg.a
CFLAGS=-std=c++14 -Wall -g `sdl2-config --cflags` -I/opt/libjpeg-turbo/include
OUT=tennis_ball_detection

$(OUT): $(SOURCES) $(HEADERS)
	g++ -o $@ $(CFLAGS) $(SOURCES) $(LFLAGS)
