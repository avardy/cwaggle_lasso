CC=clang++
CFLAGS=-O3 -std=c++17 -MMD -MP
LDFLAGS=-L/opt/homebrew/opt/sfml@2/lib -lsfml-graphics -lsfml-window -lsfml-system
INCLUDES=-I./include/ -I./src/utils/ -I/opt/homebrew/opt/sfml@2/include

# Source files
SRC_LASSO=$(wildcard src/lasso/*.cpp) 
OBJ_LASSO=$(SRC_LASSO:.cpp=.o)
DEP_LASSO=$(SRC_LASSO:.cpp=.d)

# Header files
HEADERS_INCLUDE=$(wildcard include/*.hpp)
HEADERS_LASSO=$(wildcard src/lasso/*.hpp)
HEADERS_UTILS=$(wildcard src/utils/*.hpp)
ALL_HEADERS=$(HEADERS_INCLUDE) $(HEADERS_LASSO) $(HEADERS_UTILS)

all: cwaggle_lasso

cwaggle_lasso:$(OBJ_LASSO) Makefile
	$(CC) $(OBJ_LASSO) -o ./bin/$@ $(LDFLAGS)

# Make object files depend on all headers (fallback)
$(OBJ_LASSO): $(ALL_HEADERS)

.cpp.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

# Include automatic dependency files if they exist
-include $(DEP_LASSO)

clean:
	rm -f $(OBJ_LASSO) $(DEP_LASSO) bin/cwaggle_lasso
