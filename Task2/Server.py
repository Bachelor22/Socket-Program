import socket
import random
import time

SERVER_IP = '0.0.0.0'  # 绑定到所有可用的接口
SERVER_PORT = 8080
DROP_RATE = 0.5  # 20%的丢包率

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    print(f"Server started at {SERVER_IP}:{SERVER_PORT}")

    while True:
        request, client_address = server_socket.recvfrom(1024)
        seq_no, version, _ = request.decode().split(',')
        seq_no = int(seq_no)
        version = int(version)

        if random.random() > DROP_RATE:
            server_time = time.strftime("%H-%M-%S")
            response = f"{seq_no},{version},{server_time}".encode()
            server_socket.sendto(response, client_address)
            print(f"Responded to {client_address} with seq_no: {seq_no}, server_time: {server_time}")
        else:
            print(f"Dropped packet with seq_no: {seq_no}")

if __name__ == "__main__":
    main()
