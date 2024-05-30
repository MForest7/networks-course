
#include <arpa/inet.h>
#include <array>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <random>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

constexpr size_t PAYLOAD_SIZE = 15;
using Buffer = std::array<char, PAYLOAD_SIZE>;

struct sockaddr_in make_addr(uint32_t ip, uint32_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    return addr;
}

enum net_error {
    NO_ERROR,
    TIMED_OUT,
    PROTOCOL_ERROR,
    CONN_FAILED,
    CHECKSUM_FAILED,
    UNKNOWN,
};

struct SwSocket {
    SwSocket(uint32_t ip, uint16_t port, struct timeval timeout) : dist_(0, 1) {
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in addr = make_addr(ip, port);
        bind(sockfd_, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }

    template <typename Iterator>
    net_error send_data(struct sockaddr *to, Iterator begin, Iterator end) {
        for (int i = 0; begin < end; i++) {
            size_t length = PAYLOAD_SIZE;
            if (end - begin < length)
                length = end - begin;
            Buffer buf;
            std::copy(begin, begin + length, buf.begin());
            std::cout << length << ' ' << (length == end - begin) << std::endl;
            auto result = send_single(to, buf, length, i & 1, length == end - begin);
            if (result != net_error::NO_ERROR)
                return result;
            begin += length;
        }
        return net_error::NO_ERROR;
    }

    std::optional<std::vector<char>> receive_data(struct sockaddr *from) {
        receive_status status;
        std::vector<char> buf;
        for (int i = 0;; i ^= 1) {
            status = receive_from(from, 0, i);
            if (status.error != net_error::NO_ERROR) {
                return std::nullopt;
            }
            std::cout << status.length << ' ' << status.error << std::endl;
            std::cout << "received: " << std::string{status.buffer.begin(), status.buffer.begin() + status.length} << std::endl;
            std::copy(status.buffer.begin(), status.buffer.begin() + status.length, std::back_inserter(buf));
            if (status.last)
                break;
        }
        return buf;
    }

    net_error send_single(struct sockaddr *to, Buffer data, int length, int number, int last) {
        for (int i = 0; i < 10; i++) {
            send(to, number, 0, data, last, length);
            if (receive(1, number).error == net_error::NO_ERROR) {
                return net_error::NO_ERROR;
            }
        }
        return net_error::PROTOCOL_ERROR;
    }

    void send(struct sockaddr *to, int number, int ack, Buffer buf, int last = 0, int length = PAYLOAD_SIZE) {
        if (lost())
            return;
        std::array<char, PAYLOAD_SIZE + 1> packet;
        packet[0] = (length << 4) | (last << 2) | (ack << 1) | (number << 0);
        std::copy(buf.begin(), buf.end(), packet.begin() + 1);
        sendto(sockfd_, packet.data(), PAYLOAD_SIZE + 1, 0, to, sizeof(*to));
    }

    struct receive_status {
        Buffer buffer;
        net_error error;
        uint32_t number;
        uint32_t ack;
        uint32_t last;
        uint32_t length;
    };

#define CATCH_ERROR(condition, err)    \
    if (condition) {                   \
        status.error = net_error::err; \
        return status;                 \
    }

    receive_status receive(int ack, int number) {
        std::array<unsigned char, PAYLOAD_SIZE + 1> received_data;
        receive_status status;
        status.error = net_error::UNKNOWN;
        if (recv(sockfd_, received_data.data(), PAYLOAD_SIZE + 1, 0) == -1) {
            CATCH_ERROR(errno == ETIMEDOUT, TIMED_OUT);
            CATCH_ERROR(true, UNKNOWN);
        } else {
            status.number = (received_data[0] >> 0) & 1;
            status.ack = (received_data[0] >> 1) & 1;
            status.last = (received_data[0] >> 2) & 1;
            status.length = ((size_t)received_data[0]) >> 4;

            CATCH_ERROR(number != status.number, PROTOCOL_ERROR);
            CATCH_ERROR(ack != status.ack, PROTOCOL_ERROR);

            status.error = net_error::NO_ERROR;
            std::copy(received_data.begin() + 1, received_data.end(), status.buffer.begin());
            return status;
        }
    }

#undef CATCH_ERROR

    receive_status receive_from(struct sockaddr *from, int ack, int number) {
        for (int i = 0; i < 10; i++) {
            auto status = receive(ack, number);
            if (status.error == net_error::NO_ERROR) {
                for (int j = 0; j < 10; j++) {
                    send(from, number, 1, status.buffer);
                }
                return status;
            }
        }
        receive_status status;
        status.error = net_error::PROTOCOL_ERROR;
        return status;
    }

    bool lost() {
        return dist_(rng_) < 0.3;
    }

    int sockfd_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_;
};