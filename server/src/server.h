#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <sys/epoll.h>
#include "marks.h"

//const data
#define DEFAULT_PORT 21
#define DEFAULT_ROOT "/tmp"
#define BUFFER_SIZE 8192
#define MAX_CLIENT 10
#define MAX_PORT 65535
#define MIN_PORT 20000
#define MAX_EPOLL 256

//listenfd
extern int listenfd;

//client info

typedef enum client_mode{ NO, PORT, PASV} client_mode;

struct client_info {

    //basic
    int in_use;
    char username[BUFFER_SIZE];
    int connfd;
    int epfd;

    //statistics
    int file_num;
    int byte_num;
    int current_file_byte_num;

    //status
    char root[BUFFER_SIZE];
    int has_username;
    int has_password;
    int has_login;
    client_mode mode;

    //transfer files
    int data_connfd;
    FILE* file;

    char PORT_ip[16];
    int PORT_port;

    int PASV_listenfd;
    int PASV_port;
};

extern struct client_info* client_info_list;

//server.c
void serve_client(struct client_info* info, int fd);

//command.c
int command_USER(int fd, char* parameter, struct client_info* info);
int command_PASS(int fd, char* parameter, struct client_info* info);
int command_SYST(int fd, char* parameter, struct client_info* info);
int command_TYPE(int fd, char* parameter, struct client_info* info);
int command_PORT(int fd, char* parameter, struct client_info* info);
int command_PASV(int fd, char* parameter, struct client_info* info);
int command_RETR(int fd, char* parameter, struct client_info* info);
int command_STOR(int fd, char* parameter, struct client_info* info);
int command_MKD(int fd, char* parameter, struct client_info* info);
int command_CWD(int fd, char* parameter, struct client_info* info);
int command_LIST(int fd, char* parameter, struct client_info* info);
int command_RMD(int fd, char* parameter, struct client_info* info);
int command_PWD(int fd, char* parameter, struct client_info* info);
int command_QUIT(int fd, char* parameter, struct client_info* info);
int command_UNKNOWN(int fd, char* parameter, struct client_info* info);

//util.c
int listen_port(int port);
int connect_port(char* ip, int port);
int send_response(int fd, char* sentence);
void send_file(struct client_info* info);
void recv_file(struct client_info* info);
void read_request(int fd, char* buf);
void parse_request(char* buf, char* verb, char* param);
void parse_mix_address(char* mix_address, char* ip, int* port);
void get_local_ip(int fd, char* ip);
void convert_path_format(char* source, char* target);
void get_absolute_directory(char* origin, char* target, char* current_root);
int send_file_stat(int fd, struct stat* status, char* name);
int send_directory_content(int fd, DIR* dir, char* directory);
int check_username(char* username);
int validate_user(char* username, char* password);
struct client_info* create_client_info(int fd, char* root, int epfd);
struct client_info* is_command_connect(int fd);
struct client_info* is_data_connect(int fd);
int close_client_info(int fd);
void add_epoll_event(int epfd, int fd, int state);
void delete_epoll_event(int epfd, int fd, int state);
