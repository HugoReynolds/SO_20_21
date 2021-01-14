all: server client

server: bin/aurrasd

client: bin/aurras

bin/aurrasd: obj/aurrasd.o src/structures.c
	gcc -g obj/aurrasd.o -o bin/aurrasd src/structures.c

obj/aurrasd.o: src/aurrasd.c src/structures.h
	gcc -Wall -c src/aurrasd.c -o obj/aurrasd.o

bin/aurras: obj/aurras.o src/structures.h
	gcc -g obj/aurras.o -o bin/aurras

obj/aurras.o: src/aurras.c src/structures.h
	gcc -Wall -c src/aurras.c -o obj/aurras.o

clean:
	rm bin/aurras bin/aurrasd obj/* tmp/*

test:
	bin/aurras transform samples/sample-1.mp3 tmp/sample-1.mp3 eco alto baixo
	bin/aurras transform samples/sample-2.mp3 tmp/sample-2.mp3 alto rapido baixo lento
