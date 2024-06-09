#include "../include/Channel.h"
#include "../src/Channel.cpp"
#include <iostream>

int main(){
    Channel channel(1, 2, nullptr, nullptr, nullptr, nullptr);
    std::cout << "The channel's fd is: " << channel.getSocket() << std::endl;
    return 0;
}