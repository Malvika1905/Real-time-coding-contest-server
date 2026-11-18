#include "protocol.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <cstring>
#include <iostream>

bool send_exact(int socket_fd, const void* buffer, size_t length) {
    size_t total_sent = 0;
    const char* ptr = static_cast<const char*>(buffer);

    while (total_sent < length) {
#ifdef _WIN32
        int sent = ::send(socket_fd, ptr + total_sent, static_cast<int>(length - total_sent), 0);
#else
        ssize_t sent = ::send(socket_fd, ptr + total_sent, length - total_sent, MSG_NOSIGNAL);
#endif
        if (sent <= 0) {
            return false;
        }
        total_sent += sent;
    }
    return true;
}

bool recv_exact(int socket_fd, void* buffer, size_t length) {
    size_t total_received = 0;
    char* ptr = static_cast<char*>(buffer);

    while (total_received < length) {
#ifdef _WIN32
        int recvd = ::recv(socket_fd, ptr + total_received, static_cast<int>(length - total_received), 0);
#else
        ssize_t recvd = ::recv(socket_fd, ptr + total_received, length - total_received, 0);
#endif
        if (recvd <= 0) {
            return false; // Connection closed or error
        }
        total_received += recvd;
    }
    return true;
}

bool send_packet(int socket_fd, MessageType type, StatusCode status, const std::string& payload) {
    PacketHeader header;
    header.magic = htonl(PROTOCOL_MAGIC);
    header.msg_type = htons(static_cast<uint16_t>(type));
    header.status = static_cast<uint8_t>(status);
    header.payload_len = htonl(static_cast<uint32_t>(payload.size()));

    if (!send_exact(socket_fd, &header, sizeof(header))) {
        return false;
    }

    if (!payload.empty()) {
        if (!send_exact(socket_fd, payload.data(), payload.size())) {
            return false;
        }
    }
    return true;
}

bool recv_packet(int socket_fd, PacketHeader& header, std::string& payload) {
    PacketHeader raw_header;
    if (!recv_exact(socket_fd, &raw_header, sizeof(raw_header))) {
        return false;
    }

    header.magic = ntohl(raw_header.magic);
    header.msg_type = ntohs(raw_header.msg_type);
    header.status = raw_header.status;
    header.payload_len = ntohl(raw_header.payload_len);

    if (header.magic != PROTOCOL_MAGIC) {
        std::cerr << "[Protocol Error] Invalid magic header: " << std::hex << header.magic << "\n";
        return false;
    }

    if (header.payload_len > 10 * 1024 * 1024) { // Protect against unbounded memory allocation (10MB max)
        std::cerr << "[Protocol Error] Payload too large: " << header.payload_len << "\n";
        return false;
    }

    if (header.payload_len > 0) {
        payload.resize(header.payload_len);
        if (!recv_exact(socket_fd, &payload[0], header.payload_len)) {
            return false;
        }
    } else {
        payload.clear();
    }

    return true;
}
