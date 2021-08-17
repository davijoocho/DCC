
dcc: program.o lexer.o parser.o type_check.o compiler0.o
	gcc -o dcc program.o lexer.o parser.o type_check.o compiler0.o

program.o: ./src/program.c
	gcc -c -I./include ./src/program.c

lexer.o: ./src/lexer.c ./include/lexer.h
	gcc -c -I./include ./src/lexer.c

parser.o: ./src/parser.c ./include/parser.h
	gcc -c -I./include ./src/parser.c

type_check.o: ./src/type_check.c ./include/type_check.h
	gcc -c -I./include ./src/type_check.c

compiler0.o: ./src/compiler0.c ./include/compiler0.h
	gcc -c -I./include ./src/compiler0.c

clean:
	rm *.o dcc

