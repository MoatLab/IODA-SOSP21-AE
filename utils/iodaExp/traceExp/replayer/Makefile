OPT := -lpthread -g

all: replayer

replayer: replayer.o
	gcc -o replayer replay.o ${OPT}

writer: writer.c
	gcc -o writer writer.c ${OPT}

replayer.o:
	gcc -c replay.c ${OPT}

clean:
	rm -rf *.o replayer writer
