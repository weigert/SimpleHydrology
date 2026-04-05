UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CC = clang++ -std=c++20 -DTINYENGINE_OS_MAC
TINYOS = -framework OpenGL
LF = -I/opt/homebrew/include -L/opt/homebrew/lib -L$(HOME)/.local/lib
TINYLINK = -lpthread -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lGLEW -lboost_filesystem
else
CC = g++-10 -std=c++20
TINYOS =
LF = -I$(HOME)/.local/include -L$(HOME)/.local/lib
TINYLINK = -lX11 -lpthread -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lGL -lGLEW -lboost_system -lboost_filesystem
endif

CF = -Wfatal-errors -O2

all: SimpleHydrology.cpp
			$(CC) SimpleHydrology.cpp $(CF) $(LF) -lTinyEngine $(TINYOS) $(TINYLINK) -o hydrology
