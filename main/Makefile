include ../config.in

all: testortga

testortga: testortga.c es_core.o
	$(CC) $(CFLAGS) -lm -o testortga testortga.c es_core.o
	strip testortga
	cp testortga ../bin

es_core.o: es_core.c 
	$(CC) $(CFLAGS) -c es_core.c

clean:
	rm *.o testortga
