#include "swsocket.h"
#include <iostream>

int main() {
    struct timeval TIMEOUT;
    TIMEOUT.tv_sec = 10;
    TIMEOUT.tv_usec = 0;

    SwSocket sock(0, 3001, TIMEOUT);
    auto from_addr = make_addr(0, 3000);

    auto data = sock.receive_data(reinterpret_cast<struct sockaddr *>(&from_addr));

    std::cout << std::string{data.value().begin(), data.value().end()} << std::endl;

    sock.send_data(reinterpret_cast<struct sockaddr *>(&from_addr), data.value().begin(), data.value().end());
}