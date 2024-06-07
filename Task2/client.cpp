#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <numeric>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 256
#define TIMEOUT 100 // 超时时间，单位：毫秒

struct MyApplicationPacket {
    uint16_t sequence_number;
    uint8_t ver; // Version number field
    char payload[BUFFER_SIZE - sizeof(uint16_t) - sizeof(uint8_t)];
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <Server IP> <Server Port>" << std::endl;
        exit(EXIT_FAILURE);
    }

    const char* server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    MyApplicationPacket send_packet, recv_packet;
    int addr_len = sizeof(server_addr);

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    std::vector<double> rtts;
    int received_packets = 0;

    for (int i = 1; i <= 12; ++i) {
        send_packet.sequence_number = i;
        send_packet.ver = 2; // Setting version number to 2
        snprintf(send_packet.payload, sizeof(send_packet.payload), "Request %d", i);

        bool ack_received = false;
        for (int retries = 0; retries < 3 && !ack_received; ++retries) {
            auto start = std::chrono::high_resolution_clock::now();
            sendto(sockfd, reinterpret_cast<const char*>(&send_packet), sizeof(send_packet), 0, reinterpret_cast<sockaddr*>(&server_addr), addr_len);

            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sockfd, &fds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = TIMEOUT * 1000; // microseconds

            int activity = select(0, &fds, NULL, NULL, &timeout);
            if (activity > 0 && FD_ISSET(sockfd, &fds)) {
                recvfrom(sockfd, reinterpret_cast<char*>(&recv_packet), sizeof(recv_packet), 0, reinterpret_cast<sockaddr*>(&server_addr), &addr_len);

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> rtt = end - start;

                if (recv_packet.sequence_number == send_packet.sequence_number) {
                    ack_received = true;
                    received_packets++;
                    rtts.push_back(rtt.count());
                    std::cout << "Sequence no: " << recv_packet.sequence_number
                        << ", Server IP: " << server_ip
                        << ", Port: " << server_port
                        << ", RTT: " << rtt.count() << " ms"
                        << ", Server Time: " << recv_packet.payload << std::endl;
                }
            }
        }

        if (!ack_received) {
            std::cout << "Sequence no: " << send_packet.sequence_number << ", Request time out" << std::endl;
        }
    }

    std::cout << "\n【汇总信息】" << std::endl;
    std::cout << "接收到的 UDP packets 数目: " << received_packets << std::endl;
    std::cout << "丢包率: " << static_cast<double>(12 - received_packets) / 12 * 100 << " %" << std::endl;

    if (!rtts.empty()) {
        double max_rtt = *std::max_element(rtts.begin(), rtts.end());
        double min_rtt = *std::min_element(rtts.begin(), rtts.end());
        double sum_rtt = std::accumulate(rtts.begin(), rtts.end(), 0.0);
        double avg_rtt = sum_rtt / rtts.size();

        double variance = 0.0;
        for (auto rtt : rtts) {
            variance += pow(rtt - avg_rtt, 2);
        }
        double std_dev = sqrt(variance / rtts.size());

        std::cout << "最大 RTT: " << max_rtt << " ms" << std::endl;
        std::cout << "最小 RTT: " << min_rtt << " ms" << std::endl;
        std::cout << "平均 RTT: " << avg_rtt << " ms" << std::endl;
        std::cout << "RTT 的标准差: " << std_dev << " ms" << std::endl;
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
