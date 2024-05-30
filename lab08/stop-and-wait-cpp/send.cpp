#include "swsocket.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;

int main() {
    struct timeval TIMEOUT;
    TIMEOUT.tv_sec = 1;
    TIMEOUT.tv_usec = 0;

    auto to_addr = make_addr(0, 3001);
    SwSocket sock(0, 3000, TIMEOUT);

    std::string data;
    {
        std::ifstream t("data.md");
        std::stringstream buffer;
        buffer << t.rdbuf();
        data = buffer.str();
    }
    std::cout << data << std::endl;

    sock.send_data(reinterpret_cast<struct sockaddr *>(&to_addr), data.begin(), data.end());

    {
        auto data = sock.receive_data(reinterpret_cast<struct sockaddr *>(&to_addr));

        std::cout << std::string{data.value().begin(), data.value().end()} << std::endl;
    }
}