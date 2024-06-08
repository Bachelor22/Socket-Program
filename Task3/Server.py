import socket
import threading
import sys


def handle_client(client_socket):
    # 接收初始化消息
    init_message = client_socket.recv(1024).decode()
    message_type, N = init_message.split('|')

    if message_type != "1":
        print("错误: 未收到初始化消息")
        client_socket.close()
        return

    # 发送同意消息
    agree_message = "2|".encode()
    client_socket.sendall(agree_message)

    # 处理反转请求
    for _ in range(int(N)):
        request_message = client_socket.recv(1024).decode()
        request_type, length, data = request_message.split('|', 2)

        if request_type != "3":
            print("错误: 未收到反转请求消息")
            client_socket.close()
            return

        reversed_data = data[::-1]
        response_message = f"4|{len(reversed_data)}|{reversed_data}".encode()
        client_socket.sendall(response_message)

    client_socket.close()


def main(server_port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('', int(server_port)))
    server_socket.listen(5)
    print("服务器正在监听端口", server_port)

    while True:
        client_socket, addr = server_socket.accept()
        print("连接来自", addr)
        client_handler = threading.Thread(target=handle_client, args=(client_socket,))
        client_handler.start()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("用法: python server.py <server_port>")
        sys.exit(1)
    main(sys.argv[1])
