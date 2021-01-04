
lovelace_c: lovelace.o lexer.o token.o
	gcc -o lovelace_c lovelace.o lexer.o token.o

lovelace.o: ./src/lovelace.c
	gcc -c -I./include ./src/lovelace.c

lexer.o: ./src/lexer.c ./include/lexer.h
	gcc -c -I./include ./src/lexer.c

token.o: ./src/token.c ./include/token.h
	gcc -c -I./include ./src/token.c

clean:
	rm *.o lovelace_c

