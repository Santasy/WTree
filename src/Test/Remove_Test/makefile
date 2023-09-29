CPP=g++ -std=c++17 -Wno-cpp -fmax-errors=1
FLAGS=-O3 -Wall -DVERBOSE -DNDEBUG
PROFILEFLAGS=-g -O0 -Wall -DVERBOSE
LWTREE=../../wtree/*.cpp
WTO=WTree*.o

all: clean remove

remove: Remove_Test.cpp
	$(CPP) $(FLAGS) Remove_Test.cpp $(LWTREE) -o remove_test.out
	@echo "Compilacion terminada"
	@echo

remote: clean Remove_Test.cpp
	@$(CPP) $(FLAGS) Remove_Test.cpp ../WTree.a -o remove_test.out

dbg: Remove_Test.cpp
	$(CPP) $(PROFILEFLAGS) -ggdb Remove_Test.cpp $(LWTREE) -o dbg.out
	@echo "Compilacion para debug terminada"
	@echo

clean:
	@rm -f *.out