CC=clang++
CFLAGS=-O3 -std=c++17
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system
INCLUDES=-I./include/ -I./src/utils/
SRC_EXAMPLE=$(wildcard src/example/*.cpp) 
OBJ_EXAMPLE=$(SRC_EXAMPLE:.cpp=.o)
SRC_LASSO=$(wildcard src/lasso/*.cpp) 
OBJ_LASSO=$(SRC_LASSO:.cpp=.o)

all: cwaggle_lasso

cwaggle_example:$(OBJ_EXAMPLE) Makefile
	$(CC) $(OBJ_EXAMPLE) -o ./bin/$@ $(LDFLAGS)

cwaggle_lasso:$(OBJ_LASSO) Makefile
	$(CC) $(OBJ_LASSO) -o ./bin/$@ $(LDFLAGS)

.cpp.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm $(OBJ_EXAMPLE) $(OBJ_LASSO) bin/cwaggle_example bin/cwaggle_lasso
