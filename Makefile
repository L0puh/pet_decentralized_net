main: 
	g++ main.cpp crypto.cpp node.cpp -o main 
run: main 
	./main :9001
