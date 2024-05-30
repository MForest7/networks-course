import socket
import subprocess

serverPort = 3001

serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serverSocket.bind(('', serverPort))
serverSocket.listen(1)

while True:
    connection, addr = serverSocket.accept()
    command = connection.recv(1024).decode()
    process = subprocess.run(command.split(), stdout=subprocess.PIPE)
    connection.send(process.stdout)
    connection.close()
    #while (exitCode := process.poll()) is not None:
    #    output = process.stdout.readline()
    #    connection.send(f'>> {output}'.encode())
    #connection.send(f'EXIT {exitCode}'.encode())