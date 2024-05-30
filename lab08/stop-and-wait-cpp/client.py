import socket

clientSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
clientSocket.bind(('', 3001))

while True:
    message, address = clientSocket.recvfrom(1024)
    print(address, message)