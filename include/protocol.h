#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <string>
#include <vector>

// Magic bytes to verify packet integrity in stream
constexpr uint32_t PROTOCOL_MAGIC = 0x4F535052; // "OSPR" in hex

// Protocol Command / Message Types
enum MessageType : uint16_t {
    MSG_HEARTBEAT           = 0x0001,
    MSG_LOGIN               = 0x0010,
    MSG_LOGIN_RESP          = 0x0011,
    MSG_LIST_PROBLEMS       = 0x0020,
    MSG_LIST_PROBLEMS_RESP  = 0x0021,
    MSG_GET_PROBLEM         = 0x0030,
    MSG_GET_PROBLEM_RESP    = 0x0031,
    MSG_SUBMIT_CODE         = 0x0040,
    MSG_SUBMIT_RESP         = 0x0041,
    MSG_GET_LEADERBOARD     = 0x0050,
    MSG_GET_LEADERBOARD_RESP= 0x0051,
    MSG_RACE_STRESS_TEST    = 0x0060,
    MSG_RACE_STRESS_RESP    = 0x0061,
    MSG_DEADLOCK_DEMO       = 0x0070,
    MSG_DEADLOCK_DEMO_RESP  = 0x0071,
    MSG_MEMORY_DEMO         = 0x0080,
    MSG_MEMORY_DEMO_RESP    = 0x0081,
    MSG_DISK_DEMO           = 0x0090,
    MSG_DISK_DEMO_RESP      = 0x0091,
    MSG_DISCONNECT          = 0x00FE,
    MSG_ERROR               = 0x00FF
};

// Response Status Codes
enum StatusCode : uint8_t {
    STATUS_OK               = 0x00,
    STATUS_UNAUTHORIZED     = 0x01,
    STATUS_INVALID_REQUEST  = 0x02,
    STATUS_PROBLEM_NOT_FOUND= 0x03,
    STATUS_SERVER_BUSY      = 0x04,
    STATUS_INTERNAL_ERROR   = 0x05
};

// Fixed-size Header for TCP Packet Framing
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t magic;         // Must be PROTOCOL_MAGIC
    uint16_t msg_type;      // MessageType
    uint8_t  status;        // StatusCode
    uint32_t payload_len;   // Length of following variable payload
};
#pragma pack(pop)

// Network send/receive helper declarations
bool send_exact(int socket_fd, const void* buffer, size_t length);
bool recv_exact(int socket_fd, void* buffer, size_t length);
bool send_packet(int socket_fd, MessageType type, StatusCode status, const std::string& payload);
bool recv_packet(int socket_fd, PacketHeader& header, std::string& payload);

#endif // PROTOCOL_H
