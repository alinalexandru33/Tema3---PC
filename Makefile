CFLAGS = -g -lm -Wall

all: build

build: tema3.c
	gcc tema3.c $(CFLAGS) -o bmp

run: bmp
	./bmp

clean:
	rm -fr bmp
