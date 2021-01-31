
gossip_c: gossip.o lexer.o token.o ast.o parser.o compiler.o
	gcc -o gossip_c gossip.o lexer.o token.o ast.o parser.o compiler.o

gossip.o: ./src/gossip.c
	gcc -c -I./include ./src/gossip.c

lexer.o: ./src/lexer.c ./include/lexer.h
	gcc -c -I./include ./src/lexer.c

token.o: ./src/token.c ./include/token.h
	gcc -c -I./include ./src/token.c

ast.o: ./src/ast.c ./include/ast.h
	gcc -c -I./include ./src/ast.c

parser.o: ./src/parser.c ./include/parser.h
	gcc -c -I./include ./src/parser.c

compiler.o: ./src/compiler.c ./include/compiler.h
	gcc -c -I./include ./src/compiler.c

clean:
	rm *.o lovelace_c

