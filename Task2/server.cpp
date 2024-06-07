#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 256
#define DROP_RATE 0.3 // 模拟丢包率

std::mutex console_mutex; // 控制台输出互斥锁
std::condition_variable cv; // 条件变量用于通知线程有新的客户端连接

struct MyApplicationPacket {
    uint16_t sequence_number;
    uint8_t ver; // Version number field
    char payload[BUFFER_SIZE - sizeof(uint16_t) - sizeof(uint8_t)];
};

void handleClient(SOCKET sockfd, struct sockaddr_in* client_addr, int client_len) {
    MyApplicationPacket recv_packet;
    MyApplicationPacket send_packet;

    srand(static_cast<unsigned>(time(NULL)));

    while (true) {
        int recv_len = recvfrom(sockfd, reinterpret_cast<char*>(&recv_packet), sizeof(recv_packet), 0, reinterpret_cast<sockaddr*>(client_addr), &client_len);
        if (recv_len < 0) {
            console_mutex.lock();
            std::cerr << "Failed to receive packet" << std::endl;
            console_mutex.unlock();
            continue;
        }

        // 打印接收到的数据包信息
        console_mutex.lock();
        std::cout << "Received packet from " << inet_ntoa(client_addr->sin_addr) << ":" << ntohs(client_addr->sin_port)
            << " Sequence no: " << recv_packet.sequence_number << std::endl;
        console_mutex.unlock();

        // 模拟丢包
        if ((rand() / (double)RAND_MAX) < DROP_RATE) {
            console_mutex.lock();
            std::cout << "Dropped packet from " << inet_ntoa(client_addr->sin_addr) << ":" << ntohs(client_addr->sin_port)
                << " Sequence no: " << recv_packet.sequence_number << std::endl;
            console_mutex.unlock();
            continue;
        }

        // 填充系统时间和其他内容
        std::time_t now = std::time(0);
        std::tm* ltm = localtime(&now);
        char time_str[100];
        snprintf(time_str, sizeof(time_str), "Server Time: %02d-%02d-%02d %02d:%02d:%02d",
            ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

        send_packet.sequence_number = recv_packet.sequence_number;
        send_packet.ver = recv_packet.ver; // Forwarding version number from received packet
        memset(send_packet.payload, 0, sizeof(send_packet.payload));
        snprintf(send_packet.payload, sizeof(send_packet.payload), "%s | Filler data", time_str);

        sendto(sockfd, reinterpret_cast<const char*>(&send_packet), sizeof(send_packet), 0, reinterpret_cast<sockaddr*>(client_addr), client_len);
    }
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error code: " << WSAGetLastError() << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started. Waiting for clients..." << std::endl;

    while (true) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        std::thread(handleClient, sockfd, &client_addr, client_len).detach();
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
