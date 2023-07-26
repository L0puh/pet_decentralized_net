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
    size_t message_size;
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
        void handle_send(uint16_t port);
        void handle_command(std::string message, uint16_t port_me);
        std::string message_parse(std::string message);
        int connect_to(std::string port);
        std::string message_get();
    public:

        void send_to(std::string message, uint16_t port_me, uint16_t port_to);
        void send_all(std::string message, uint16_t port_me);
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
    uint16_t u_port = stoi(port);
    while(true){
        std::thread send_th(&Node::handle_send, this, u_port);
        send_th.detach();
        int sockfd_new = accept(sockfd, (sockaddr*)&their_addr, &their_addrlen);

        //FIXME: save the correct port.
        uint16_t port_conn = ntohs(their_addr.sin_port);

        Conn_t conn{.id = id, .sockfd = sockfd_new}; 
        id++;
        connections.push_back(conn);
        std::thread recv_th(&Node::handle_recv, this, sockfd_new, &their_addr);
        printf("thread's up!\n");
        recv_th.detach();
    }
}
Node::~Node(){
    close(sockfd);
    exit(1);
}
void Node::handle_recv(int sockfd, const sockaddr_in *addr){
    int bytes, pckg_size;
    printf("handle recv\n");
    Package pckg; 
    
    while((bytes = recv(sockfd, &pckg, sizeof(pckg), 0)) > 0){
        char *msg = new char[pckg.message_size + 1];
        msg[pckg.message_size] = '\0';
        handle_error(recv(sockfd, msg, pckg.message_size, 0));
        printf("[%s:%d]: %s\n", inet_ntoa(addr->sin_addr), pckg.from, msg);
        delete[] msg;
    }
    handle_error(bytes);
    if (bytes == 0 ){
        printf("user's disconnected\n");
        close(sockfd);
        //TODO: delete user from connections
    }
    
}

void Node::connections_print(){
    printf("[+] your connections: \n");
    for (std::vector<Conn_t>::iterator itr = connections.begin();
            itr != connections.end(); itr++){
        printf("%d : %hu \n", itr->id, itr->port);
    }

}
void Node::handle_command(std::string message, uint16_t port_me){
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
        } else if(msg == "/send"){
            std::string port = message_parse(message);
            sockfd = connect_to(port);
            uint16_t port_to = stoi(port);
            send_to(message, port_me, port_to);
        }

    }
}
void Node::handle_send(uint16_t port){
    int sockfd=0;
    Package pckg;
    while (true) {
        std::string message = message_get();
        if (message.at(0) == '/') {
            handle_command(message, port);
        } else {
            send_all(message, port);
        }
    }
}

void Node::send_to(std::string message, uint16_t port_me, uint16_t port_to){
    printf("send from %d to %d: %s\n", port_me, port_to, message.c_str());
    return;
}
void Node::send_all(std::string message, uint16_t port_me){
    Package pckg {.from=port_me, .message_size = message.size()};

    for (std::vector<Conn_t>::iterator itr = connections.begin();
        itr != connections.end(); itr++){
            pckg.to = itr->port;
            handle_error(send(itr->sockfd, &pckg, sizeof(pckg), 0));
            handle_error(send(itr->sockfd, message.c_str(), message.size(), 0));

         }
    return;
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
