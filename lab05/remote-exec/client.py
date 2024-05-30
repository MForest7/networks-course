import socket
import sys

port = int(sys.argv[1])
command = sys.argv[2]

clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
clientSocket.bind(('', 3000))
clientSocket.connect(('', port))
clientSocket.send(command.encode())
response = clientSocket.recv(1024).decode()
print(response)