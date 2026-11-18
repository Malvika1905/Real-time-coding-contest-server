#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>

// Checks if the incoming socket buffer is an HTTP request (GET, POST, OPTIONS, etc.)
bool is_http_request(const char* buffer, size_t length);

// Handles and dispatches an HTTP request, sending a JSON response with CORS headers
bool handle_http_connection(int client_fd, const std::string& request_raw);

#endif // HTTP_SERVER_H
