TINYLINK = -lX11 -lpthread -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lGL -lGLEW -lboost_system -lboost_filesystem

CC = g++-10 -std=c++20 -ggdb3
CF = -Wfatal-errors -O2
LF = -I/usr/local/include -L/usr/local/lib -lnoise

all: SimpleHydrology.cpp
			$(CC) SimpleHydrology.cpp $(CF) $(LF) -lTinyEngine $(TINYLINK) -o hydrology
