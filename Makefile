CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lSDL2

SRC = main/main.cpp
OUT = bin/app

all: $(OUT)

$(OUT): $(SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

clean:
	rm -rf bin

.PHONY: all clean
