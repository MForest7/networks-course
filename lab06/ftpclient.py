from ftplib import FTP
import sys

ftp = FTP('ftp.dlptest.com')
ftp.login("dlpuser", "rNrKYTX9g7z3RgJRmxWuGHbeu")

if sys.argv[1] == 'ls':
    dir = sys.argv[2] if len(sys.argv) > 2 else ''
    print('> ' + '\n> '.join(ftp.nlst(dir)))
elif sys.argv[1] == 'store':
    filename_local = sys.argv[2]
    filename_remote = sys.argv[3]
    print('> ' + ftp.storbinary(f'STOR {filename_remote}', open(filename_local, 'rb')))
elif sys.argv[1] == 'retrieve':
    filename_local = sys.argv[2]
    filename_remote = sys.argv[3]
    print('> ' + ftp.retrbinary(f'RETR {filename_remote}', open(filename_local, 'wb').write))