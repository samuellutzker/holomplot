OS := $(shell uname -s)
OBJ = canvas.o window.o

ifeq ($(OS),Darwin)  # macOS
	CXXFLAGS = -O -std=c++20 -stdlib=libc++ `wx-config --cxxflags`
    LDFLAGS = -O -std=c++20 -stdlib=libc++ `wx-config --cxxflags --libs core base gl` -framework IOKit -framework Carbon -framework Cocoa -framework OpenGL -lglew -ltbb
else # ifeq ($(OS),Linux)  # Linux
	CXXFLAGS = -O -std=c++20 -stdlib=libc++ `wx-config --cxxflags` -D IGNORE_GLEW_INIT_RET
    LDFLAGS = -O -std=c++17 -Wl,--copy-dt-needed-entries `wx-config --cxxflags --libs core base gl` -lGLEW -ltbb
endif

.PHONY: clean

plot: $(OBJ)
	g++ $(OBJ) $(LDFLAGS) -o plot

expr: expr-test.cpp expr.hpp
	g++ -O -std=c++20 -stdlib=libc++ expr-test.cpp -o expr

window.o: window.cpp window.h canvas.h
	g++ $(CXXFLAGS) -c window.cpp -o window.o

canvas.o: canvas.cpp canvas.h buffers.hpp shader.hpp expr.hpp window.h
	g++ $(CXXFLAGS) -c canvas.cpp -o canvas.o

clean:
	rm -f $(OBJ)

delete:
	rm -f $(OBJ) plot expr
