#ifndef NODE_H
#define NODE_H

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

#endif
