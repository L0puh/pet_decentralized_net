#include "node.h"
#include <cstdint>
int Node::init_socket(std::string port){ 
    
    struct sockaddr_in servinfo;  
    servinfo.sin_family = AF_INET;
    servinfo.sin_port = htons(stoi(port));

    int sockfd = socket(servinfo.sin_family, SOCK_DGRAM, 0);
    handle_error(bind(sockfd, (const struct sockaddr *)&servinfo, sizeof(servinfo)) == -1);
    

    printf("[PORT:%s | ", port.c_str());
    printf("LIST OF COMMANDS]\n/connect :PORT\n/send :PORT\n/quit\n/show\n[start the chat]\n");

    return sockfd;
}

Node::Node(std::string port){
    sockfd = init_socket(port); 
    uint16_t u_port = stoi(port);

    std::thread send_th(&Node::handle_send, this, u_port, sockfd);
    send_th.detach();
    handle_recv(sockfd);

}

void Node::handle_recv(int sockfd){

    int bytes;
    Package_t pckg;
    struct sockaddr_in their_addr;

    memset(&their_addr, 0, sizeof(their_addr));
    socklen_t their_addrlen = sizeof(their_addr);

    while ((bytes = recvfrom(sockfd, &pckg, sizeof(pckg), 0, (struct sockaddr *)&their_addr, &their_addrlen)) != -1 ){
        if (pckg.type == REQ_CONN) {
            long secret_key = generate_number();
            long open_key = generate_key(pckg.crypto, secret_key);
        
            ulong session_key = get_key();
            Package_t pckg_new = Package_t{.addr={.from=pckg.addr.to, .to = pckg.addr.from}, .type=CONN_BACK, .crypto= pckg.crypto};
            pckg_new.crypto.open_key = open_key;
            send_to(pckg_new);
            connection_add(pckg.addr, session_key); 

        } else if (pckg.type == CONN_BACK) {
            ulong session_key = (long)pow(pckg.crypto.open_key, secret_key) % pckg.crypto.mod; 
            connection_add(pckg.addr, session_key); 

        } else if (pckg.type == MESSAGE) {
            char *buff = new char[pckg.addr.message_size + 1];
            buff[pckg.addr.message_size] = '\0';

            handle_error(recvfrom(sockfd, buff, pckg.addr.message_size, 0, (struct sockaddr *)&their_addr, &their_addrlen) > 0);
            std::string message = code(buff, connection_get_key(pckg.addr.from));
            printf("[%s:%d]: %s\n", inet_ntoa(their_addr.sin_addr), pckg.addr.from, message.c_str());
            std::lock_guard<std::mutex> lock(mtx);
            memset(&their_addr, 0, sizeof(their_addr));
            delete[] buff;

        } else if (pckg.type == DISCONN) { 
            printf("[ ! ] user %d is disconnected \n", pckg.addr.from);
            delete_user(pckg.addr.from);
            continue;
        }
    } 
    handle_error(bytes);
}
Node::~Node(){
    close(sockfd);
    exit(1);
}

void Node::handle_send(uint16_t port, int sockfd){
    Message_t pckg;
    while (true) {
        std::string message = message_get();
        message += '\n';
        if (message.at(0) == '/') {
            handle_command(message, port);
        } else if (message == "\n"){
            continue;
        } else {
            send_all(message, port);
        }
    }
}


/// SEND ///

struct sockaddr_in get_addr(uint16_t port_to ){
    struct sockaddr_in their_addr;
    
    memset(&their_addr, 0, sizeof(their_addr));
    
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(port_to); 
    return their_addr;
}

void Node::send_to(std::string message, uint16_t port_me, uint16_t port_to){
    if (message.back() == '\n') {
        message.pop_back();
    }
    std::string c_message = code(message, connection_get_key(port_to));
    Message_t addr {.from=port_me, .to = port_to, .message_size = c_message.size()} ;
    Package_t pckg = {.addr = addr, .type = MESSAGE};

    struct sockaddr_in their_addr = get_addr(port_to);
    socklen_t their_addrlen = sizeof(their_addr);
    
    handle_error(sendto(sockfd, &pckg, sizeof(pckg), 0, (const struct sockaddr*)&their_addr, their_addrlen));
    handle_error(sendto(sockfd, c_message.c_str(), c_message.size(), 0, (const struct sockaddr*)&their_addr, their_addrlen));
    
    return;
}

void Node::send_to(Package_t pckg){
   struct sockaddr_in their_addr = get_addr(pckg.addr.to);
   socklen_t their_addrlen = sizeof(their_addr);
   handle_error(sendto(sockfd, &pckg, sizeof(pckg), 0, (const struct sockaddr*)&their_addr, their_addrlen));
}

void Node::send_all(std::string message, uint16_t port_me){
    for (std::vector<Conn_t>::iterator itr = connections.begin();
        itr != connections.end(); itr++){
            send_to(message, port_me, itr->port);
         }
    return;
}


/// MESSAGE ///
std::string Node::message_parse(std::string message){
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


void Node::handle_command(std::string message, uint16_t port_me){
    int i=0, j; bool flag = true;
    if (message == "/quit\n") {
        disconnect(port_me);
    }
    else if(message == "/show\n") connections_print();
    else {
        for (const char c: message){
            if (c == ' ') break;
            else if (c == '\n') flag=false;
            i++;
        }
        std::string msg = message, port;
        if (flag) msg.erase(i);
       
        if (msg == "/connect"){
            port = message_parse(message);
            connect_to(port, port_me);
        
        } else if(msg == "/send"){
            port = message_parse(message);
            for (j=0; port[j] != ' '; j++);

            std::string message_to_send = port;
            message_to_send.erase(0, j+1);
            
            uint16_t port_to = stoi(port);
            send_to(message_to_send, port_me, port_to);
        }
    }
}

/// CONNECTION ///
ulong Node::connection_get_key(uint16_t port) {
    if (connections.size() != 0 ) {
        for (std::vector<Conn_t>::iterator itr = connections.begin(); itr != connections.end(); itr++ ){
            if (itr->port == port) {
                return itr->session_key;
            }
        }
    }
    return 0;
}


int Node::connection_add(Message_t addr, ulong session_key) {
    std::vector<Conn_t>::iterator itr = connections.begin();
    while (itr != connections.end()) {
        if (itr->port == addr.from) {
            return 1;
        }
        itr++;
    }
    connections.push_back(Conn_t{.id = id, .port = addr.from, .session_key = session_key});
    id++;
    return 0;
}
void Node::delete_user(uint16_t port_from){
    std::vector<Conn_t>::iterator itr = connections.begin();
    while (itr!=connections.end()){
        if (port_from == itr->port){
            connections.erase(itr);
            break;
        }
        itr++;
    }
}
void Node::disconnect(uint16_t port_me){
    std::vector<Conn_t>::iterator itr = connections.begin();
    while(itr != connections.end()){
        send_to(Package_t{.addr={.from=port_me, .to=itr->port}, .type=DISCONN});
        itr++;
    }
    exit(0);
}
int Node::connect_to(std::string port, uint16_t port_me){
    uint16_t u_port = stoi(port);
    bool exsist = false;
    
    
    std::vector<Conn_t>::iterator itr = connections.begin();
    while(itr != connections.end()){
        if (itr->port == u_port){
            exsist=true;
            break;
        } 
        itr++;
    }
    if (!exsist || connections.size() == 0)  {
        secret_key = generate_number();  
        Package_t pckg = {
            .addr= 
                {
                    .from = port_me, 
                    .to=u_port, 
                    .message_size=0
                }, 
            .type = REQ_CONN, 
            .crypto = 
                {
                .mod = generate_number(), 
                .prim = generate_number()
                }
        };
        pckg.crypto.open_key = (long)pow(pckg.crypto.prim, secret_key) % pckg.crypto.mod;
        send_to(pckg);
        printf("connected a to port: %s\n", port.c_str());
    }
    return 0;
}
void Node::connections_print(){
    printf("[+] your connections: \n");
    for (std::vector<Conn_t>::iterator itr = connections.begin();
            itr != connections.end(); itr++){
        printf("%d : %hu | %lu \n", itr->id, itr->port, itr->session_key);
    }

}

///

void Node::handle_error(int result){
    if (result == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}
