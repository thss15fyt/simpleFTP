import socket
import getpass
import random

# const
size = 8192
commands = ["open", "user", "type", "binary", "system",
            "passive", "get", "put", "ls", "dir",
            "mkdir", "cd", "rmdir", "pwd", "exit",
            "quit", "bye", "help"]
min_port = 20000
max_port = 65535


# client info
global local_ip
global connect_socket
global connect_flag
global port_listen_socket
global data_socket
global transfer_type    # 1:binary
global mode    # 1:port 2:passive


# util functions

def init():
    global connect_flag, transfer_type, mode
    connect_flag = False
    transfer_type = 1
    mode = 1


def get_local_ip():
    global local_ip
    local_ip = input("Please input your ip address: ")


def parse_input(line):
    space = line.find(' ')
    if space == -1:
        return line, ''
    else:
        return line[0:space], line[space+1:len(line)]


def recv_response():
    global connect_socket
    content = connect_socket.recv(size).decode('utf-8')
    print(content, end="")
    return content


def send_request(content):
    global connect_socket
    connect_socket.send(bytes(content, 'utf-8'))


def data_connect():
    global port_listen_socket, data_socket, ip_address

    if mode == 1:
        port_listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        while True:
            random_port = int(random.random() * (max_port - min_port) + min_port)
            try:
                port_listen_socket.bind((local_ip, random_port))
            except socket.error:
                continue
            break
        port_listen_socket.listen(1)
        send_request("PORT %s,%d,%d\r\n" % (local_ip.replace('.', ','), random_port / 256, random_port % 256))
        response = recv_response()
        if not response.startswith(("200")):
            return False
    elif mode == 2:
        send_request("PASV\r\n")
        response = recv_response()
        if not response.startswith(("227")):
            return False
        begin = response.find(',') - 1
        end = response.rfind(',') + 1
        while response[begin].isdigit():
            begin -= 1
        begin += 1
        while response[end].isdigit():
            end += 1
        ip_and_port = response[begin:end]
        split_result = ip_and_port.split(',')
        ip_address = '.'.join(split_result[0:4])
        port = int(split_result[4]) * 256 + int(split_result[5])
        data_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        data_socket.connect((ip_address, port))

    return True


def data_connect_close():
    global port_listen_socket, data_socket
    data_socket.close()
    if mode == 1:
        port_listen_socket.close()


def connect_required():
    global connect_flag
    print(connect_flag)
    if connect_flag:
        return True
    else:
        print("Not connected.")
        return False


# command functions
def command_open(parameter):
    global connect_socket
    params = parameter.split(' ')
    connect_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if len(params) == 0 or len(params[0]) == 0:
        print("Required ip address")
        return
    ip_address = params[0]
    port = 21
    try:
        if len(params) > 1:
            port = int(params[1])
        connect_socket.connect((ip_address, port))
        print("Connected to " + ip_address)
        response = recv_response()
        if not response.startswith("220"):
            print("Connection refused")
        global connect_flag
        connect_flag = True
    except:
        print("Connection refused")


def command_user(parameter):
    if not connect_required():
        return

    if len(parameter) > 0:
        send_request("USER %s\r\n" % (parameter))
    else:
        username = input("Name: ")
        send_request("USER %s\r\n" % (username))
    response = recv_response()
    if not response.startswith(("331")):
        print("Login failed")
        return
    password = getpass.getpass("Password: ")
    send_request("PASS %s\r\n" % (password))
    response = recv_response()
    if not response.startswith(("230")):
        print("Login failed")
        return


def command_type(parameter):
    if not connect_required():
        return

    if transfer_type == 1:
        print("Using binary mode to transfer files.")
    else:
        print("Unknown type mode.")


def command_binary(parameter):
    if not connect_required():
        return

    global transfer_type
    transfer_type = 1
    send_request("TYPE I\r\n")
    recv_response()


def command_system(parameter):
    if not connect_required():
        return

    send_request("SYST\r\n")
    recv_response()


def command_passive(parameter):
    global mode
    if mode == 1:
        mode = 2
        print("Passive mode on.")
    else:
        mode = 1
        print("Passive mode off")


def command_get(parameter):
    if not connect_required():
        return

    global data_socket
    if not data_connect():
        return
    send_request("RETR %s\r\n" % (parameter))
    if mode == 1:
        data_socket, addr = port_listen_socket.accept()
    response = recv_response()
    if not response.startswith(("150")):
        return
    file = open(parameter, 'wb')
    while True:
        content = data_socket.recv(size)
        if len(content) == 0:
            break
        file.write(content)
    file.close()
    data_connect_close()
    recv_response()


def command_put(parameter):
    if not connect_required():
        return

    try:
        file = open(parameter, 'rb')
    except FileNotFoundError:
        print("No suck file: %s" % (parameter))
        return

    global data_socket
    if not data_connect():
        return
    send_request("STOR %s\r\n" % (parameter))
    if mode == 1:
        data_socket, addr = port_listen_socket.accept()
    response = recv_response()
    if not response.startswith(("150")):
        return

    while True:
        content = file.read(size)
        if len(content) == 0:
            break
        data_socket.send(content)
    file.close()
    data_connect_close()
    recv_response()


def command_list(parameter):
    if not connect_required():
        return

    global data_socket
    if not data_connect():
        return
    send_request("LIST\r\n")
    if mode == 1:
        data_socket, addr = port_listen_socket.accept()
    response = recv_response()
    if not response.startswith(("150")):
        return
    while True:
        content = data_socket.recv(size)
        if len(content) == 0:
            break
        print(content.decode('utf-8'), end="")
    data_connect_close()
    response = recv_response()
    if not response.startswith(("226")):
        print("Failed to get list.")


def command_mkdir(parameter):
    if not connect_required():
        return

    send_request("MKD %s\r\n" % (parameter))
    recv_response()


def command_cwd(parameter):
    if not connect_required():
        return

    send_request("CWD %s\r\n" % (parameter))
    recv_response()


def command_rmdir(parameter):
    if not connect_required():
        return

    send_request("RMD %s\r\n" % (parameter))
    recv_response()


def command_pwd(parameter):
    if not connect_required():
        return

    send_request("PWD\r\n")
    recv_response()


def command_quit(parameter):
    global connect_flag
    if connect_flag:
        send_request("QUIT\r\n")
        recv_response()


def command_help(parameter):
    print("Commands may be abbreviated.  Commands are:")
    for command in commands:
        print(command)


# commands -> functions
command_functions = {
    "open": command_open,
    "user": command_user,
    "type": command_type,
    "binary": command_binary,
    "system": command_system,
    "passive": command_passive,
    "get": command_get,
    "put": command_put,
    "ls": command_list,
    "dir": command_list,
    "mkdir": command_mkdir,
    "cd": command_cwd,
    "rmdir": command_rmdir,
    "pwd": command_pwd,
    "exit": command_quit,
    "quit": command_quit,
    "bye": command_quit,
    "help": command_help,
}


def main():

    init()
    get_local_ip()

    while True:
        line = input("ftp> ")

        if len(line) == 0:
            continue

        command, parameter = parse_input(line)
        if command in commands:
            command_functions[command](parameter)
        else:
            print("Can't resolve your command")

        if command in ["exit", "quit", "bye"]:
            break

    if connect_flag:
        connect_socket.close()


main()
