#include "node.h"

int Node::init_socket(std::string port){ 
    
    struct sockaddr_in servinfo;  
    servinfo.sin_family = AF_INET;
    servinfo.sin_port = htons(stoi(port));

    int sockfd = socket(servinfo.sin_family, SOCK_DGRAM, 0);
    handle_error(bind(sockfd, (const struct sockaddr *)&servinfo, sizeof(servinfo)) == -1);
    
    printf("[port: %s] ready to chat\n", port.c_str() );
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


/// SEND ///
void Node::send_to(std::string message, uint16_t port_me, uint16_t port_to){
    Package pckg {.from=port_me, .to = port_to, .message_size = message.size()} ;
    struct sockaddr_in their_addr;
    
    memset(&their_addr, 0, sizeof(their_addr));
    
    their_addr.sin_family = AF_INET;
    socklen_t their_addrlen = sizeof(their_addr);
    their_addr.sin_port = htons(port_to); 

    handle_error(sendto(sockfd, &pckg, sizeof(pckg), 0, (const struct sockaddr*)&their_addr, their_addrlen));
    handle_error(sendto(sockfd, message.c_str(), message.size(), 0, (const struct sockaddr*)&their_addr, their_addrlen));
    
    return;
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
    int i=0;
    if (message == "/quit") exit(0);
    else if(message == "/show") connections_print();
    else {
        while (message.at(i) != ' ') i++;
        std::string msg = message;
        msg.erase(i);
        i=0;
        std::string port = message_parse(message);
        if (msg == "/connect"){
            connect_to(port);
        } else if(msg == "/send"){
            //send :PORT message 
            int j;
            for (j=0; port[j] != ' '; j++);
            std::string message_to_send = port;
            message_to_send.erase(0, j+1);
            uint16_t port_to = stoi(port);
            send_to(message_to_send, port_me, port_to);
        }
    }
}

/// CONNECTION ///
int Node::connect_to(std::string port){
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
        connections.push_back(Conn_t{.id=id, .port=u_port});
        id++;
        printf("connected to port: %s\n", port.c_str());
    }
    return 0;
}
void Node::connections_print(){
    printf("[+] your connections: \n");
    for (std::vector<Conn_t>::iterator itr = connections.begin();
            itr != connections.end(); itr++){
        printf("%d : %hu \n", itr->id, itr->port);
    }

}

///

void Node::handle_error(int result){
    if (result == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}
