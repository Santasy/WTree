CPP=g++ -std=c++17 -Wall -Wno-unused-variable
FLAGS=-O3 -DVERBOSE -DNDEBUG
PROFILEFLAGS=-ggdb -O0 -DVERBOSE
LWTREE=../../wtree/*.cpp

all: clean rules

rules: Rules_Test.cpp
	$(CPP) $(FLAGS) Rules_Test.cpp RulesTestNamespace.cpp $(LWTREE) -o rules_test.out
	@echo "Compilacion terminada"
	@echo

remote: clean Rules_Test.cpp
	@$(CPP) $(FLAGS) Rules_Test.cpp RulesTestNamespace.cpp ../WTree.a -o rules_test.out

dbg: Rules_Test.cpp
	$(CPP) $(PROFILEFLAGS) Rules_Test.cpp RulesTestNamespace.cpp $(LWTREE) -o dbg.out
	@echo "Compilacion para debug terminada"
	@echo

clean:
	@(rm *.out || true)