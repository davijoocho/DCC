
dcc: program.o lexer.o parser.o type_check.o
	gcc -o dcc program.o lexer.o parser.o type_check.o

program.o: ./src/program.c
	gcc -c -I./include ./src/program.c

lexer.o: ./src/lexer.c ./include/lexer.h
	gcc -c -I./include ./src/lexer.c

parser.o: ./src/parser.c ./include/parser.h
	gcc -c -I./include ./src/parser.c

type_check.o: ./src/type_check.c ./include/type_check.h
	gcc -c -I./include ./src/type_check.c

clean:
	rm *.o dcc

