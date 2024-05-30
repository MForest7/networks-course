import socket
import datetime
import time

serverPort = 4200

serverSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
serverSocket.bind(('', serverPort))
serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
while True:
    serverSocket.sendto(bytes(f'Time {datetime.datetime.now()}', 'utf-8'), ("255.255.255.255", 3001))
    time.sleep(1)