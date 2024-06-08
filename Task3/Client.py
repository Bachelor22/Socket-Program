import socket
import sys
import random


def read_ascii_file(file_path):
    with open(file_path, 'r') as file:
        return file.read()


def split_file_content(content, Lmin, Lmax):
    segments = []
    while content:
        length = random.randint(Lmin, Lmax)
        segments.append(content[:length])
        content = content[length:]
    return segments


def main(server_ip, server_port, file_path, Lmin, Lmax):
    # 读取并分割文件内容
    content = read_ascii_file(file_path)
    segments = split_file_content(content, Lmin, Lmax)

    # 建立与服务器的连接
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((server_ip, int(server_port)))

        # 发送初始化消息
        N = len(segments)
        init_message = f"1|{N}".encode()
        client_socket.sendall(init_message)

        # 等待同意消息
        response = client_socket.recv(1024)
        if response.decode() != "2|":
            print("错误: 未收到同意消息")
            return

        # 发送反转请求并接收反转答案
        reversed_content = []
        for i, segment in enumerate(segments):
            request_message = f"3|{len(segment)}|{segment}".encode()
            client_socket.sendall(request_message)

            response = client_socket.recv(1024)
            response_type, response_length, reversed_segment = response.decode().split('|', 2)
            if response_type != "4":
                print("错误: 未收到反转答案消息")
                return
            reversed_content.append(reversed_segment)
            print(f"第{i + 1}块: {reversed_segment}")

        # 保存最终反转的内容到文件
        with open("reversed_output.txt", 'w') as output_file:
            output_file.write(''.join(reversed_content))


if __name__ == "__main__":
    if len(sys.argv) != 6:
        print("用法: python client.py <server_ip> <server_port> <file_path> <Lmin> <Lmax>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5]))
