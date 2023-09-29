CPP=g++ -std=c++17
FLAGS=-O3 -Wall -DVERBOSE -DNDEBUG
PROFILEFLAGS=-g -O0 -Wall -DVERBOSE
LWTREE=../../wtree/*.cpp
WTO=WTree*.o

all: clean search

search: Search_Test.cpp
	$(CPP) $(FLAGS) Search_Test.cpp $(LWTREE) -o search_test.out
	@echo "Compilacion terminada"
	@echo

remote: clean Search_Test.cpp
	@$(CPP) $(FLAGS) Search_Test.cpp ../WTree.a -o search_test.out

dbg: Search_Test.cpp
	$(CPP) $(PROFILEFLAGS) -ggdb Search_Test.cpp $(LWTREE) -o dbg.out
	@echo "Compilacion para debug terminada"
	@echo

clean:
	@(rm *.out || true)