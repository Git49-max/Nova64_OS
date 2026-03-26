CXX = g++
LLVM_CXXFLAGS = $(shell llvm-config --cxxflags | sed 's/-fno-exceptions//')
CXXFLAGS = -std=c++17 -Wall -Iinclude $(LLVM_CXXFLAGS)

# Configurações de Linkagem
LDFLAGS_DYNAMIC = $(shell llvm-config --ldflags --system-libs --libs all)
LIBS = -lncurses

SRCS = src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp src/analysis.cpp
TARGET = n++

# Compilação padrão (Desenvolvimento)
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS_DYNAMIC) $(LIBS) -o $(TARGET) -O3

# Compilação de Release (Híbrida - Maior portabilidade)
release: $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) \
	/usr/lib/gcc/x86_64-linux-gnu/14/libstdc++.a \
	$(LDFLAGS_DYNAMIC) $(LIBS) \
	-static-libgcc -o $(TARGET) -O3

clean:
	rm -f $(TARGET) output.o output

.PHONY: clean release