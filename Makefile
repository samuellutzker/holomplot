OS := $(shell uname -s)
OBJ = canvas.o window.o

ifeq ($(OS),Darwin)  # macOS
	CXXFLAGS = -O -std=c++20 -stdlib=libc++ `wx-config --cxxflags` -I/opt/homebrew/include
	LDFLAGS = -O `wx-config --cxxflags --libs core base gl` -framework IOKit -framework Carbon -framework Cocoa -framework OpenGL -L/opt/homebrew/lib -lGLEW -ltbb
else # ifeq ($(OS),Linux)  # Linux
	CXXFLAGS = -O -std=c++20 `wx-config --cxxflags` -D IGNORE_GLEW_INIT_RET
	LDFLAGS = -O -Wl,--copy-dt-needed-entries `wx-config --cxxflags --libs core base gl` -lGLEW -ltbb
endif

.PHONY: clean

plot: $(OBJ)
	g++ -Wall -Wpedantic $(OBJ) $(LDFLAGS) -o plot

expr: expr-test.cpp expr.hpp
	g++ -O -Wall -Wpedantic $(CXXFLAGS) expr-test.cpp -o expr

window.o: window.cpp window.h canvas.h
	g++ -Wall -Wpedantic $(CXXFLAGS) -c window.cpp -o window.o

canvas.o: canvas.cpp canvas.h buffers.hpp shader.hpp expr.hpp window.h
	g++ -Wall -Wpedantic $(CXXFLAGS) -c canvas.cpp -o canvas.o

clean:
	rm -f $(OBJ)

remove:
	rm -f $(OBJ) plot expr
