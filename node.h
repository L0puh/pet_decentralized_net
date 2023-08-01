#ifndef NODE_H
#define NODE_H

#include <atomic>
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
#include <math.h>
#include <random>

enum types {
    REQ_CONN=0,
    CONN_BACK,
    CONN_DENY,
    MESSAGE,
    DISCONN,
};
struct Conn_t {
    int id;
    uint16_t port;
    ulong session_key; 
};
struct Message_t {
    uint16_t from; 
    uint16_t to;
    size_t message_size;
};

struct Crypto_t {
    long mod;
    long prim;
    long open_key;
};

struct Package_t {
    Message_t addr;
    uint8_t type;
    Crypto_t crypto;
};

class Crypto{
    private: 
        ulong session_key;
        const ulong MAX = 99999999999;
        const int MIN = 10000;
    protected:
        long generate_number();
        long generate_key( Crypto_t crypto, ulong secret_key );
        ulong get_key();
        std::string code(std::string message, ulong session_key);
};

class Node : public Crypto {
    private:
        int id = 0;
        int sockfd;
        ulong secret_key;
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
        int connect_to(std::string port, uint16_t port_me);
        std::string message_get();
        int connection_add(Message_t addr, ulong session_key);
        void disconnect(uint16_t port_me);
        ulong connection_get_key(uint16_t port);
        void delete_user(uint16_t port_from);
    public:
        void send_to(std::string message, uint16_t port_me, uint16_t port_to);
        void send_to(Package_t pckg);
        void send_all(std::string message, uint16_t port_me);
    public:
        void handle_send(uint16_t port, int sockfd);
        void handle_recv(int sockfd);
};


#endif
