CC = g++
CFLAGS = -Wall -Wextra -std=gnu++23 $(if $(release),-O3,-g)
INCLUDE = -I/usr/local/include/kissfft
LDLIBS = -lkissfft-float-openmp -lsndfile -lportaudio

compile:
	mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDE) $(LDLIBS) src/main.cpp -o bin/termviz

install: compile
	sudo cp bin/termviz /usr/local/bin
