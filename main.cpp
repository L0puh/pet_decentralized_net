#include <iterator>
#include <sys/socket.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <vector>
#include <string>
#include <thread>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

const int BACK_LOG = 10;

struct Conn_t {
    int id;
    int sockfd;
};
struct Package {
    int from; 
    int to;
    std::string message;
};

class Node {
    private:
        std::string port;
        int id = 0;
        int sockfd;
        std::vector<Conn_t> connections;
    public:
        Node(std::string port);
        ~Node();
    public:
        struct addrinfo* init_servinfo();
        void handle_error(int result);
        void init_node(); 
        void handle_recv();
        void handle_send();
};

struct addrinfo* Node::init_servinfo(){
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port.c_str(), &hints, &servinfo);

    if (res == -1){
        fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(res));
        exit(1);
    }
    return servinfo;
}
void Node::init_node(){ 
    
    struct addrinfo *servinfo = init_servinfo();
    
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    handle_error(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1);
    freeaddrinfo(servinfo);
    
    handle_error(listen(sockfd, BACK_LOG) == -1);

    printf("server's up\n");
    return;
}
void Node::handle_error(int result){
    if (result == -1){
        fprintf(stderr, "error: %s", strerror(errno));
        exit(1);
    }
}
Node::Node(std::string port) {
    port = port;
    init_node();

    struct sockaddr_in their_addr;
    socklen_t their_addrlen = sizeof(their_addr);
    
    while(true){
        int sockfd_new = accept(sockfd, (sockaddr*)&their_addr, &their_addrlen);
        Conn_t conn{.id = id, .sockfd = sockfd_new};
        id++;
        connections.push_back(conn);
        std::thread recv_th(&Node::handle_recv, this);
        std::thread send_th(&Node::handle_send, this);
        recv_th.detach();
        send_th.detach();
    }
}
Node::~Node(){
    close(sockfd);
    exit(1);
}
void Node::handle_recv(){

}
void Node::handle_send(){

}

int main (int argc, char* argv[]) {
    if(argc == 1){
        printf("error: define a port\n");
        exit(1);
    }
    std::string port = argv[1];
    Node node(port);
}
