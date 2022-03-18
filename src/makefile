
flags = -std=c++17 -pedantic
objects = symbol.o symtable.o emitter.o compiler.o parser.o lexer.o main.o 
all = $(objects) pca lexer.cpp parser.hpp parser.cpp

pca: $(objects)
	g++  $(flags) -o pca $(objects) -lfl -g

lexer.cpp: lexer.l parser.hpp
	flex lexer.l

parser.cpp parser.hpp: parser.y
	bison -d parser.y

compiler.o: compiler.cpp compiler.hpp emitter.hpp
	g++ $(flags) -c compiler.cpp

symtable.o: symtable.cpp symtable.hpp symbol.hpp compilerexception.hpp
	g++ $(flags) -c symtable.cpp

emitter.o: emitter.cpp emitter.hpp symtable.hpp
	g++ $(flags) -c emitter.cpp

lexer.o: lexer.cpp
	g++ $(flags) -c lexer.cpp

main.o: main.cpp global.hpp
	g++ $(flags) -c main.cpp

parser.o: parser.cpp parser.hpp
	g++ $(flags) -c parser.cpp

symbol.o: symbol.cpp symbol.hpp enums.hpp utils.hpp
	g++ $(flags) -c symbol.cpp

clean:
	rm -f $(all)

.PHONY : clean