CXX   = g++-8
EXE = iplC
CXXDEBUG = -g -Wall
CXXSTD = -std=c++11


.PHONY: all parser lexer clean
all: parser lexer 	
	g++-8 $(CXXDEBUG) -o iplC driver.cpp parser.o scanner.o ast.hh type.hh symtab.hh symtab.cc type.cc ast.cc

parser: parser.yy scanner.hh 
	bison -d -v $<
	g++-8 $(CXXDEBUG) -c parser.tab.cc -o parser.o 

lexer: scanner.l scanner.hh parser.tab.hh parser.tab.cc	
	flex++ --outfile=scanner.yy.cc  $<
	g++-8  $(CXXDEBUG) -c scanner.yy.cc -o scanner.o

clean:
	rm parser.o scanner.o iplC
	rm parser.output parser.tab.cc parser.tab.hh
	rm scanner.yy.cc
	rm stack.hh location.hh position.hh