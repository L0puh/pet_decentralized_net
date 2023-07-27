#include "node.h"

int main (int argc, char* argv[]) {
    if(argc == 1){
        printf("error: define a port\n");
        exit(1);
    }
    std::string port = argv[1];
    port.erase(port.begin());
    Node node(port);
}
