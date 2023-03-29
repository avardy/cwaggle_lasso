CC=clang++
CFLAGS=-O3 -std=c++17
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system
INCLUDES=-I./include/ -I./src/utils/
SRC_LASSO=$(wildcard src/lasso/*.cpp) 
OBJ_LASSO=$(SRC_LASSO:.cpp=.o)

all: cwaggle_lasso

cwaggle_lasso:$(OBJ_LASSO) Makefile
	$(CC) $(OBJ_LASSO) -o ./bin/$@ $(LDFLAGS)

.cpp.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm $(OBJ_LASSO) bin/cwaggle_lasso
