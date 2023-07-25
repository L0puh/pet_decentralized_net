#include <cstdint>
#include <iostream>
#include <iterator>
#include <netinet/in.h>
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
#include <arpa/inet.h>
const int BACK_LOG = 10;

struct Conn_t {
    int id;
    int sockfd;
    uint16_t port;
};
struct Package {
    int from; 
    int to;
    std::string message;
};

class Node {
    private:
        int id = 0;
        int sockfd;
        std::vector<Conn_t> connections;
    public:
        Node(std::string port);
        ~Node();
    public:
        struct addrinfo* init_servinfo(std::string port);
        void init_node(std::string port); 
        void handle_error(int result);
        void connections_print();
    public:
        void handle_send();
        void handle_command(std::string message);
        std::string message_parse(std::string message);
        int connect_to(std::string port);
        std::string message_get();
    public:
        void handle_recv(int sockfd, const sockaddr_in *addr);
};

struct addrinfo* Node::init_servinfo(std::string port){
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
void Node::init_node(std::string port){ 
    
    struct addrinfo *servinfo = init_servinfo(port);
    
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
    init_node(port);

    struct sockaddr_in their_addr;
    socklen_t their_addrlen = sizeof(their_addr);
    while(true){
        std::thread send_th(&Node::handle_send, this);
        send_th.detach();
        int sockfd_new = accept(sockfd, (sockaddr*)&their_addr, &their_addrlen);

        //FIXME: save the correct port.
        uint16_t port_conn = ntohs(their_addr.sin_port);

        Conn_t conn{.id = id, .sockfd = sockfd_new}; 
        id++;
        connections.push_back(conn);
        std::thread recv_th(&Node::handle_recv, this, sockfd_new, &their_addr);
        recv_th.detach();
    }
}
Node::~Node(){
    close(sockfd);
    exit(1);
}
void Node::handle_recv(int sockfd, const sockaddr_in *addr){
    int bytes;
    Package pckg;
    while(recv(sockfd, &pckg, sizeof(pckg), 0) > 0){
        printf("[%s]: %s\n", inet_ntoa(addr->sin_addr), pckg.message.c_str());
    }
}
void Node::connections_print(){
    printf("[+] your connections: \n");
    for (std::vector<Conn_t>::iterator itr = connections.begin();
            itr != connections.end(); itr++){
        printf("%d : %hu \n", itr->id, itr->port);
    }

}
void Node::handle_command(std::string message){
    int i=0;
    if (message == "/quit") exit(0);
    else if(message == "/show") connections_print();
    else {
        while (message.at(i) != ' ') i++;
        std::string msg = message;
        msg.erase(i);
        i=0;
        if (msg == "/connect"){
            std::string port = message_parse(message);
            sockfd = connect_to(port);
            uint16_t u_port = stoi(port);
            Conn_t new_conn{.id = id, .sockfd = sockfd, .port = u_port};
            connections.push_back(new_conn);
            id++;
          }

    }
}
void Node::handle_send(){
    int sockfd=0;
    Package pckg;
    while (true) {
        std::string message = message_get();
        if (message.at(0) == '/') {
            handle_command(message);
        } else {
            /* send_to(); */
        }
    }
}

int Node::connect_to(std::string port){
    struct sockaddr_in addr; 
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(stoi(port));

    int conn = socket(AF_INET, SOCK_STREAM, 0);
    handle_error(connect(conn, (struct sockaddr*)&addr, sizeof(addr)));
    printf("connected to port: %s\n", port.c_str());
    return conn;

}
std::string Node::message_parse(std::string message){
    // /connect :1000
    int i=0;
    while(message.at(i) != ':') i++;
    message.erase(0, i+1);
    return message;
}
std::string Node::message_get() {
    std::string msg; 
    std::getline(std::cin, msg);
    return msg;
}
int main (int argc, char* argv[]) {
    if(argc == 1){
        printf("error: define a port\n");
        exit(1);
    }
    std::string port = argv[1];
    port.erase(port.begin());
    printf("port: %s\n", port.c_str());
    Node node(port);
}
