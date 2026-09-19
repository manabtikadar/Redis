import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.connect(("127.0.0.1", 6379))

# Send only part of PING
s.sendall(b"*1\r\n$4\r\nPI")

time.sleep(1)

# Send the remaining part
s.sendall(b"NG\r\n")

print(s.recv(4096).decode())

s.close()