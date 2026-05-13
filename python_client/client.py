#!/usr/bin/env python3
"""
Descriere:
Acest fisier implementeaza un client Python pentru proiectul T17.

Clientul Python:
- se conecteaza la serverul TCP
- face LOGIN automat ca user normal
- poate trimite un fisier sursa catre server pentru analiza
- poate descarca un raport generat de server

Moduri de rulare:
  python3 python_client/client.py upload tests/sample.c
  python3 python_client/client.py download sample_report.txt
"""

import os
import socket
import sys

SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080

CLIENT_USERNAME = "user1"
CLIENT_PASSWORD = "pass1"

BUFFER_SIZE = 4096
DOWNLOAD_DIR = "downloads_py"


def send_all(sock, data):
    """
    Trimite toate datele prin socket.
    """
    sock.sendall(data)


def recv_exact(sock, size):
    """
    Primeste exact size bytes din socket.
    """
    data = b""

    while len(data) < size:
        chunk = sock.recv(size - len(data))

        if not chunk:
            raise ConnectionError("Conexiunea s-a inchis inainte de primirea tuturor datelor.")

        data += chunk

    return data


def recv_line(sock):
    """
    Citeste o linie din socket pana la caracterul newline.
    """
    data = b""

    while True:
        ch = sock.recv(1)

        if not ch:
            raise ConnectionError("Conexiunea s-a inchis inainte de newline.")

        if ch == b"\n":
            break

        data += ch

    return data.decode("utf-8", errors="replace")


def connect_to_server():
    """
    Creeaza conexiunea TCP catre server.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER_IP, SERVER_PORT))
    return sock


def receive_result_text(sock):
    """
    Primeste un raspuns de tip RESULT de la server.

    Format:
      RESULT <size>
      <payload>
    """
    line = recv_line(sock)

    if not line.startswith("RESULT "):
        raise ValueError(f"Raspuns invalid de la server: {line}")

    parts = line.split()

    if len(parts) != 2:
        raise ValueError("Header RESULT invalid.")

    size = int(parts[1])
    payload = recv_exact(sock, size)

    return payload.decode("utf-8", errors="replace")


def login_to_server(sock):
    """
    Trimite comanda LOGIN catre server.
    """
    command = f"LOGIN {CLIENT_USERNAME} {CLIENT_PASSWORD}\n"
    send_all(sock, command.encode("utf-8"))

    response = receive_result_text(sock)

    if not response.startswith("OK"):
        raise PermissionError(f"Autentificare esuata: {response}")

    return True


def upload_file(path):
    """
    Trimite un fisier catre server pentru analiza.
    """
    if not os.path.isfile(path):
        print(f"Fisierul nu exista: {path}", file=sys.stderr)
        return 1

    filename = os.path.basename(path)

    with open(path, "rb") as file:
        content = file.read()

    try:
        sock = connect_to_server()

        with sock:
            login_to_server(sock)

            header = f"UPLOAD {filename} {len(content)}\n"
            send_all(sock, header.encode("utf-8"))
            send_all(sock, content)

            response = receive_result_text(sock)
            print(response, end="")

        return 0

    except OSError as error:
        print(f"Eroare socket: {error}", file=sys.stderr)
        return 1
    except Exception as error:
        print(f"Eroare: {error}", file=sys.stderr)
        return 1


def receive_file_response(sock):
    """
    Primeste un raspuns de tip FILE sau RESULT.

    FILE:
      FILE <filename> <size>
      <file_bytes>

    RESULT:
      RESULT <size>
      <payload>
    """
    line = recv_line(sock)

    if line.startswith("RESULT "):
        parts = line.split()

        if len(parts) != 2:
            raise ValueError("Header RESULT invalid.")

        size = int(parts[1])
        payload = recv_exact(sock, size)
        print(payload.decode("utf-8", errors="replace"), end="")
        return 0

    if not line.startswith("FILE "):
        raise ValueError(f"Raspuns invalid de la server: {line}")

    parts = line.split()

    if len(parts) != 3:
        raise ValueError("Header FILE invalid.")

    filename = os.path.basename(parts[1])
    size = int(parts[2])

    os.makedirs(DOWNLOAD_DIR, exist_ok=True)

    output_path = os.path.join(DOWNLOAD_DIR, filename)

    remaining = size

    with open(output_path, "wb") as file:
        while remaining > 0:
            chunk_size = min(BUFFER_SIZE, remaining)
            chunk = recv_exact(sock, chunk_size)
            file.write(chunk)
            remaining -= len(chunk)

    print(f"Report downloaded to {output_path}")
    return 0


def download_report(report_name):
    """
    Cere serverului descarcarea unui raport.
    """
    try:
        sock = connect_to_server()

        with sock:
            login_to_server(sock)

            command = f"DOWNLOAD_REPORT {report_name}\n"
            send_all(sock, command.encode("utf-8"))

            return receive_file_response(sock)

    except OSError as error:
        print(f"Eroare socket: {error}", file=sys.stderr)
        return 1
    except Exception as error:
        print(f"Eroare: {error}", file=sys.stderr)
        return 1


def print_usage(program_name):
    """
    Afiseaza modul de utilizare.
    """
    print("Utilizare:")
    print(f"  python3 {program_name} upload <fisier_sursa>")
    print(f"  python3 {program_name} download <raport.txt>")


def main():
    if len(sys.argv) != 3:
        print_usage(sys.argv[0])
        return 1

    command = sys.argv[1]
    value = sys.argv[2]

    if command == "upload":
        return upload_file(value)

    if command == "download":
        return download_report(value)

    print_usage(sys.argv[0])
    return 1


if __name__ == "__main__":
    sys.exit(main())