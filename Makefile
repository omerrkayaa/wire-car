# Tel Araba Projesi - Derleme Dosyasi
#
# Linux:   make
# macOS:   make PLATFORM=mac
# Windows: make PLATFORM=win   (MinGW + freeglut gerekir)
#
# Windows MinGW kurulumu:
#   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-freeglut
#   make PLATFORM=win

PLATFORM ?= linux

CXX     = g++
SRC     = wire_car/main.cpp

ifeq ($(PLATFORM), mac)
    TARGET  = wire_car
    LIBS    = -framework OpenGL -framework GLUT
    CFLAGS  = -Wall -O2 -Wno-deprecated
else ifeq ($(PLATFORM), win)
    TARGET  = wire_car.exe
    LIBS    = -lopengl32 -lglu32 -lfreeglut -lm
    CFLAGS  = -Wall -O2
else
    TARGET  = wire_car
    LIBS    = -lGL -lGLU -lglut -lm
    CFLAGS  = -Wall -O2
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f wire_car wire_car.exe

.PHONY: all clean
