include ../config.in

all: hpc wrapper_hpc.o

hpc: wrapper_hpc.c hpc.o es_comm_cli.o
	$(CC) $(CFLAGS) -o hpc wrapper_hpc.c hpc.o es_comm_cli.o
	strip hpc
	cp hpc ../bin

wrapper_hpc.o: wrapper_hpc.c
	$(CC) $(CFALGS) -o wrapper_hpc.o -c wrapper_hpc.c

hpc.o: hpc.c
	$(CC) $(CFLAGS) -o hpc.o -c hpc.c

es_comm_cli.o: es_comm_cli.c
	$(CC) $(CFLAGS) -o es_comm_cli.o -c es_comm_cli.c

clean:
	rm -f *.o hpc

