CXX = g++
LLVM_CXXFLAGS = $(shell llvm-config --cxxflags | sed 's/-fno-exceptions//')
CXXFLAGS = -std=c++17 -Wall -Iinclude $(LLVM_CXXFLAGS)
LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs all)

SRCS = src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp
TARGET = n++

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -lncurses -o $(TARGET) -O3

clean:
	rm -f $(TARGET) output.o output