OBJ = canvas.o window.o
COMPFLAGS = -O -std=c++20 -stdlib=libc++ `wx-config --cxxflags`
LINKFLAGS = -O -std=c++20 -stdlib=libc++ `wx-config --cxxflags --libs core base gl` -framework IOKit -framework Carbon -framework Cocoa -framework OpenGL -lglew -ltbb

.PHONY: clean

plot: $(OBJ)
	g++ $(LINKFLAGS) $(OBJ) -o plot

expr: expr-test.cpp expr.hpp
	g++ -O -std=c++20 -stdlib=libc++ expr-test.cpp -o expr

window.o: window.cpp window.h canvas.h
	g++ $(COMPFLAGS) -c window.cpp -o window.o

canvas.o: canvas.cpp canvas.h buffers.hpp shader.hpp expr.hpp window.h
	g++ $(COMPFLAGS) -c canvas.cpp -o canvas.o

clean:
	rm -f $(OBJ)

delete:
	rm -f $(OBJ) plot expr
