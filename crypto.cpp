#include "node.h"


long Crypto::generate_number(){
    std::srand(std::time(nullptr));

    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_int_distribution<int> distr(MIN, MAX);
 
    return  distr(eng) ;
}

std::string Crypto::code(std::string message, ulong session_key) {
    std::string c_message; 
    for (int i=0; i != message.size(); i++ ){
        char sym = (char)message.at(i) ^ session_key % MIN;
        c_message += sym;
    }
    return c_message;
}

long Crypto::generate_key(Crypto_t crypto, ulong secret_key){
    
    long my_key = (long)pow(crypto.prim, secret_key) % crypto.mod; 
    session_key = (long)pow(crypto.open_key, secret_key) % crypto.mod; 

    return my_key;
}

ulong Crypto::get_key(){
    return session_key;
}
