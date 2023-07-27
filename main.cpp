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
#include <mutex>

struct Conn_t {
    int id;
    uint16_t port;
    
};
struct Package {
    uint16_t from; 
    uint16_t to;
    size_t message_size;
};

class Node {
    private:
        int id = 0;
        int sockfd;
        std::mutex mtx;
        std::vector<Conn_t> connections;
    public:
        Node(std::string port);
        ~Node();
    public:
        int init_socket(std::string port);
        void handle_error(int result);
        void connections_print();
    public:
        void handle_command(std::string message, uint16_t port_me);
        std::string message_parse(std::string message);
        int connect_to(std::string port);
        std::string message_get();
    public:
        void send_to(std::string message, uint16_t port_me, uint16_t port_to);
        void send_all(std::string message, uint16_t port_me);
    public:
        void handle_send(uint16_t port, int sockfd);
        void handle_recv(int sockfd);
};

int Node::init_socket(std::string port){ 
    
    struct sockaddr_in servinfo;  
    servinfo.sin_family = AF_INET;
    servinfo.sin_port = htons(stoi(port));

    sockfd = socket(servinfo.sin_family, SOCK_DGRAM, 0);
    handle_error(bind(sockfd, (const struct sockaddr *)&servinfo, sizeof(servinfo)) == -1);
    
    printf("[port: %s] ready to chat\n", port.c_str() );
    return sockfd;
}
void Node::handle_error(int result){
    if (result == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}
Node::Node(std::string port){
    int sockfd = init_socket(port);

    uint16_t u_port = stoi(port);

    std::thread send_th(&Node::handle_send, this, u_port, sockfd);
    send_th.detach();
    handle_recv(sockfd);

}

void Node::handle_recv(int sockfd){

    int bytes;
    Package pckg;
    struct sockaddr_in their_addr;

    memset(&their_addr, 0, sizeof(their_addr));
    socklen_t their_addrlen = sizeof(their_addr);

    while ((bytes = recvfrom(sockfd, &pckg, sizeof(pckg), 0, (struct sockaddr *)&their_addr, &their_addrlen)) != -1 ){

        char *buff = new char[pckg.message_size + 1];
        buff[pckg.message_size] = '\0';
        handle_error(recvfrom(sockfd, buff, pckg.message_size, 0, (struct sockaddr *)&their_addr, &their_addrlen) > 0);

        printf("[%s:%d]: %s\n", inet_ntoa(their_addr.sin_addr), pckg.from, buff);
        delete[] buff;
        {
            std::lock_guard<std::mutex> lock(mtx);
            connect_to(std::to_string(pckg.from));               
            memset(&their_addr, 0, sizeof(their_addr));
        } 
    }
    handle_error(bytes);
}
Node::~Node(){
    close(sockfd);
    exit(1);
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
            connect_to(port);
        } else if(msg == "/send"){
            std::string port = message_parse(message);
            connect_to(port);
            /* send_to(message, port_me, port_to); */
        }

    }
}
void Node::handle_send(uint16_t port, int sockfd){
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
    struct sockaddr_in their_addr;
    memset(&their_addr, 0, sizeof(their_addr));
    their_addr.sin_family = AF_INET;
        
    socklen_t their_addrlen = sizeof(their_addr);
    
    for (std::vector<Conn_t>::iterator itr = connections.begin();
        itr != connections.end(); itr++){
            pckg.to = itr->port;
            their_addr.sin_port = htons(itr->port);
            handle_error(sendto(sockfd, &pckg, sizeof(pckg), 0, (const struct sockaddr*)&their_addr, their_addrlen));
            handle_error(sendto(sockfd, message.c_str(), message.size(), 0, (const struct sockaddr*)&their_addr, their_addrlen));
         }

    return;
}


int Node::connect_to(std::string port){
    uint16_t u_port = stoi(port);
    bool exsist = false;
    std::vector<Conn_t>::iterator itr = connections.begin();
    while(itr != connections.end()){
        itr++;
        if (itr->port == u_port){
            exsist=true;
            break;
        } 
    }
    if (!exsist || connections.size() == 0)  {
        connections.push_back(Conn_t{.id=id, .port=u_port});
        id++;
        printf("connected to port: %s\n", port.c_str());
    }
    return 0;

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
    Node node(port);
}
