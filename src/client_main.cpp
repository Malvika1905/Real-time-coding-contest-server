#include "protocol.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

void print_banner() {
    std::cout << "\033[1;36m";
    std::cout << "========================================================================\n";
    std::cout << "               REAL-TIME CODING CONTEST CLIENT (CLI)                    \n";
    std::cout << "                 Operating Systems Lab Demonstration                    \n";
    std::cout << "========================================================================\n";
    std::cout << "\033[0m";
}

void print_menu() {
    std::cout << "\n\033[1;33m[CONTEST & OS DEMONSTRATION MENU]\033[0m\n";
    std::cout << "  1. View Available Problems (Module 8: Sockets/Client-Server)\n";
    std::cout << "  2. View Problem Details (Module 2: Request-Response Protocol)\n";
    std::cout << "  3. Submit Solution (Module 3 & 7: Fork, Exec, Pipe & Sandbox)\n";
    std::cout << "  4. View Live Leaderboard (Module 3: Mutex & RWLock Protected)\n";
    std::cout << "  5. [DEMO] Concurrency & Race Condition Test (Module 3)\n";
    std::cout << "  6. [DEMO] Deadlock Avoidance: Banker's Algorithm (Module 4)\n";
    std::cout << "  7. [DEMO] Virtual Memory: FIFO vs LRU Paging (Module 5)\n";
    std::cout << "  8. [DEMO] Disk Scheduling: FCFS, SSTF, SCAN, C-SCAN (Module 6)\n";
    std::cout << "  9. Disconnect & Exit (Module 2 & 7: Connection Teardown)\n";
    std::cout << "Enter your choice (1-9): ";
}

// Built-in demonstration code templates
const char* CODE_AC_TWO_SUM = 
"#include <iostream>\n"
"using namespace std;\n"
"int main() {\n"
"    long long a, b;\n"
"    if (cin >> a >> b) {\n"
"        cout << (a + b) << endl;\n"
"    }\n"
"    return 0;\n"
"}\n";

const char* CODE_WA_TWO_SUM = 
"#include <iostream>\n"
"using namespace std;\n"
"int main() {\n"
"    long long a, b;\n"
"    cin >> a >> b;\n"
"    cout << (a * b) << endl; // Intentionally Wrong Answer\n"
"    return 0;\n"
"}\n";

const char* CODE_TLE_INFINITE_LOOP = 
"#include <iostream>\n"
"using namespace std;\n"
"int main() {\n"
"    volatile int x = 0;\n"
"    while (true) { x++; } // Infinite loop to trigger RLIMIT_CPU\n"
"    return 0;\n"
"}\n";

const char* CODE_MLE_MEMORY_BOMB = 
"#include <iostream>\n"
"#include <vector>\n"
"using namespace std;\n"
"int main() {\n"
"    // Exceeds 64MB memory limit to trigger RLIMIT_AS or bad_alloc\n"
"    vector<int> huge(50000000, 1);\n"
"    cout << huge[0] << endl;\n"
"    return 0;\n"
"}\n";

int main(int argc, char* argv[]) {
    print_banner();

    std::string server_ip = "127.0.0.1";
    int server_port = 8080;

    if (argc >= 2) server_ip = argv[1];
    if (argc >= 3) server_port = std::stoi(argv[2]);

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

    std::cout << "[Client] Connecting to contest server at " << server_ip << ":" << server_port << "...\n";
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "[Client Error] Connection to server failed! Ensure server is running.\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }

    std::cout << "\033[1;32m[Client] Connected successfully to contest server!\033[0m\n\n";

    // Authentication Prompt
    std::string username, password;
    std::cout << "Enter your participant username: ";
    std::getline(std::cin, username);
    if (username.empty()) username = "student_1";

    std::cout << "Enter password: ";
    std::getline(std::cin, password);
    if (password.empty()) password = "pass";

    std::string login_payload = username + ":" + password;
    if (!send_packet(sock, MSG_LOGIN, STATUS_OK, login_payload)) {
        std::cerr << "Failed to send login packet.\n";
        return 1;
    }

    PacketHeader resp_hdr;
    std::string resp_data;
    if (!recv_packet(sock, resp_hdr, resp_data)) {
        std::cerr << "Server disconnected during login.\n";
        return 1;
    }
    std::cout << "\n" << resp_data << "\n";

    bool running = true;
    while (running) {
        print_menu();
        std::string choice_str;
        if (!std::getline(std::cin, choice_str)) break;
        if (choice_str.empty()) continue;

        int choice = 0;
        try { choice = std::stoi(choice_str); } catch (...) { continue; }

        switch (choice) {
            case 1: { // View Problems
                send_packet(sock, MSG_LIST_PROBLEMS, STATUS_OK, "");
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 2: { // View Problem Details
                std::cout << "Enter Problem ID (1-3): ";
                std::string pid;
                std::getline(std::cin, pid);
                send_packet(sock, MSG_GET_PROBLEM, STATUS_OK, pid);
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 3: { // Submit Solution
                std::cout << "\n--- SUBMISSION SIMULATOR ---\n";
                std::cout << "Select submission template to demonstrate OS Process Sandbox:\n";
                std::cout << "  1. Correct Solution (Accepted - AC)\n";
                std::cout << "  2. Incorrect Logic (Wrong Answer - WA)\n";
                std::cout << "  3. Infinite Loop (Demonstrates setrlimit RLIMIT_CPU Time Limit Exceeded - TLE)\n";
                std::cout << "  4. Memory Allocation Bomb (Demonstrates setrlimit RLIMIT_AS MLE/RE)\n";
                std::cout << "Choice (1-4): ";
                std::string sub_type;
                std::getline(std::cin, sub_type);

                std::string code;
                if (sub_type == "2") code = CODE_WA_TWO_SUM;
                else if (sub_type == "3") code = CODE_TLE_INFINITE_LOOP;
                else if (sub_type == "4") code = CODE_MLE_MEMORY_BOMB;
                else code = CODE_AC_TWO_SUM;

                std::string payload = "1#" + code;
                std::cout << "\n[Client] Submitting code for Problem #1 to server...\n";
                send_packet(sock, MSG_SUBMIT_CODE, STATUS_OK, payload);
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 4: { // View Leaderboard
                send_packet(sock, MSG_GET_LEADERBOARD, STATUS_OK, "");
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 5: { // Concurrency & Race Condition Experiment
                std::cout << "\nChoose Concurrency Mode to Demonstrate to Professor:\n";
                std::cout << "  1. WITH_LOCKS (Mutual Exclusion with pthread_mutex_t - 0 Lost Updates)\n";
                std::cout << "  2. WITHOUT_LOCKS (Bypasses Locks - Generates Race Condition & Lost Updates)\n";
                std::cout << "Choice (1-2): ";
                std::string race_choice;
                std::getline(std::cin, race_choice);
                std::string mode = (race_choice == "2" ? "WITHOUT_LOCKS" : "WITH_LOCKS");

                std::cout << "\n[Client] Requesting server to execute multi-threaded concurrency experiment...\n";
                send_packet(sock, MSG_RACE_STRESS_TEST, STATUS_OK, mode);
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 6: { // Banker's Algorithm Deadlock Demo
                std::cout << "\n[Client] Requesting Banker's Algorithm evaluation...\n";
                send_packet(sock, MSG_DEADLOCK_DEMO, STATUS_OK, "");
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 7: { // Paging Demo
                std::cout << "\n[Client] Requesting Virtual Memory Paging simulation (FIFO vs LRU)...\n";
                send_packet(sock, MSG_MEMORY_DEMO, STATUS_OK, "");
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 8: { // Disk Scheduling Demo
                std::cout << "\n[Client] Requesting Disk Scheduling comparison (FCFS, SSTF, SCAN, C-SCAN)...\n";
                send_packet(sock, MSG_DISK_DEMO, STATUS_OK, "");
                recv_packet(sock, resp_hdr, resp_data);
                std::cout << "\n" << resp_data << "\n";
                break;
            }
            case 9: { // Disconnect
                send_packet(sock, MSG_DISCONNECT, STATUS_OK, "");
                running = false;
                break;
            }
            default:
                std::cout << "Invalid choice. Please select 1-9.\n";
                break;
        }
    }

    std::cout << "[Client] Closing socket connection. Goodbye!\n";
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}
