"""Exercise an actual TCP connection inside a temporary network namespace."""
import json
import socket

with socket.socket() as listener:
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    with socket.create_connection(listener.getsockname(), timeout=2) as client:
        with listener.accept()[0] as server:
            client.sendall(b"E17-probe")
            received=server.recv(32)
            server.sendall(received[::-1])
            reply=client.recv(32)
            assert received==b"E17-probe" and reply==b"eborp-71E"
print(json.dumps({"result":"TCP_CONNECT_SEND_RECEIVE_PASS",
                  "address":"127.0.0.1", "bytes_each_direction":9}))
