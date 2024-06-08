import socket
import time
import random
import statistics
from datetime import datetime

SERVER_IP = '127.0.0.1'  # 需根据实际情况设置
SERVER_PORT = 8080  # 需根据实际情况设置
TIMEOUT = 0.1  # 100ms
NUM_REQUESTS = 12
VERSION = 2


def create_request(seq_no):
    return f"{seq_no},{VERSION},data".encode()


def parse_response(response):
    parts = response.decode().split(',')
    return int(parts[0]), parts[1], parts[2]  # seq_no, version, system_time


def main():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.settimeout(TIMEOUT)

    received_packets = 0
    rtt_list = []
    start_times = []
    end_times = []

    for seq_no in range(1, NUM_REQUESTS + 1):
        request = create_request(seq_no)
        attempt = 0
        success = False

        while attempt < 3 and not success:
            start_time = time.time()
            client_socket.sendto(request, (SERVER_IP, SERVER_PORT))
            try:
                response, server_address = client_socket.recvfrom(1024)
                end_time = time.time()
                rtt = (end_time - start_time) * 1000  # 转换为毫秒
                rtt_list.append(rtt)
                received_packets += 1
                seq_no_resp, ver_resp, server_time = parse_response(response)
                print(
                    f"Seq no: {seq_no_resp}, {server_address[0]}:{server_address[1]}, RTT: {rtt:.2f} ms, Server Time: {server_time}")
                start_times.append(datetime.strptime(server_time, "%H-%M-%S"))
                end_times.append(datetime.strptime(server_time, "%H-%M-%S"))
                success = True
            except socket.timeout:
                attempt += 1
                print(f"Sequence request timeout: {seq_no}")

        if not success:
            print(f"Sequence request failed after 3 attempts: {seq_no}")

    if received_packets > 0:
        max_rtt = max(rtt_list)
        min_rtt = min(rtt_list)
        avg_rtt = sum(rtt_list) / len(rtt_list)
        std_dev_rtt = statistics.stdev(rtt_list)
        server_response_time = (end_times[-1] - start_times[0]).total_seconds() if start_times and end_times else 0
        packet_loss_rate = (1 - received_packets / NUM_REQUESTS) * 100

        print("\n【汇总】")
        print(f"接收到的udp packets数目: {received_packets}")
        print(f"丢包率: {packet_loss_rate:.2f}%")
        print(f"最大RTT: {max_rtt:.2f} ms")
        print(f"最小RTT: {min_rtt:.2f} ms")
        print(f"平均RTT: {avg_rtt:.2f} ms")
        print(f"RTT的标准差: {std_dev_rtt:.2f} ms")
        print(f"Server的整体响应时间: {server_response_time:.2f} s")
    else:
        print("没有接收到任何udp packets")

    client_socket.close()


if __name__ == "__main__":
    main()
