run: 
	g++ main.cpp crypto.cpp node.cpp -o build
	./build :$(PORT)
	rm build
