
lovelace_c: lovelace.o lexer.o token.o ast.o parser.o
	gcc -o lovelace_c lovelace.o lexer.o token.o ast.o parser.o

lovelace.o: ./src/lovelace.c
	gcc -c -I./include ./src/lovelace.c

lexer.o: ./src/lexer.c ./include/lexer.h
	gcc -c -I./include ./src/lexer.c

token.o: ./src/token.c ./include/token.h
	gcc -c -I./include ./src/token.c

ast.o: ./src/ast.c ./include/ast.h
	gcc -c -I./include ./src/ast.c

parser.o: ./src/parser.c ./include/parser.h
	gcc -c -I./include ./src/parser.c

clean:
	rm *.o lovelace_c

