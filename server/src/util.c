#include "server.h"

int listen_port(int port) {

    int listenfd;
    struct sockaddr_in addr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        return -1;
    int bReuseaddr = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(int));

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("Error bind()\n");
        return -1;
    }

    if (listen(listenfd, MAX_CLIENT) == -1) {
        printf("Error listen()\n");
        return -1;
    }

    return listenfd;
}

int connect_port(char* ip, int port) {
    int connfd;
    if ((connfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        return -1;
    }

    if (connect(connfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

    return connfd;
}


int send_response(int fd, char* sentence) {
    int p = 0;
    int len = strlen(sentence);
    while (p < len) {
        int n = write(fd, sentence + p, len - p);
        if (n < 0)
            return -1;
        else
            p += n;        
    }
    return p;
}

int send_file(int fd, FILE* file) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int n_read = fread(buffer, 1, sizeof(buffer) - 1, file);
        if (n_read < 0) {
            return -1;
        }
        else if(n_read == 0) {
            break;
        }
        else {
            int n_write;
            if((n_write = write(fd, buffer, n_read)) == -1)
                return -2;
        }
    } 
    return 0;
}

int recv_file(int fd, FILE* file) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int n_read = read(fd, buffer, sizeof(buffer) - 1);
        if (n_read < 0) {
            return -1;
        }
        else if(n_read == 0) {
            break;
        }
        else {
            int n_write;
            if((n_write = fwrite(buffer, 1, n_read, file)) == -1)
                return -2;
        }
    } 
    return 0;
}

int send_file_stat(int fd, struct stat* status, char* name) {
    char response[BUFFER_SIZE];
    struct tm* time = localtime(&status->st_mtime);
    struct passwd* pwd = getpwuid(status->st_uid);
    struct group* grp = getgrgid(status->st_gid);
    char month[12][4] = {"Jan\0", "Feb\0", "Mar\0", "Apr\0", "May\0", "Jun\0",
                        "Jul\0", "Aug\0", "Sep\0", "Oct\0", "Nov\0", "Dec\0"};
    if(S_ISREG(status->st_mode))
        sprintf(response, file_format, 
            pwd->pw_name, grp->gr_name, (unsigned int)status->st_size,
            month[time->tm_mon], time->tm_mday, time->tm_hour, time->tm_min,
            name);
    else if(S_ISDIR(status->st_mode))
        sprintf(response, directory_format, 
            pwd->pw_name, grp->gr_name, (unsigned int)status->st_size,
            month[time->tm_mon], time->tm_mday, time->tm_hour, time->tm_min,
            name);
    int send_status = send_response(fd, response);
    if(send_status < 0)
        return send_status;
    else
        return 0;
}

int send_directory_content(int fd, DIR* dir, char* directory) {
    struct dirent* file;
    while((file = readdir(dir)) != NULL) {
        char file_directory[BUFFER_SIZE];
        sprintf(file_directory, "%s/%s", directory, file->d_name);
        struct stat status;
        stat(file_directory, &status);
        int send_status = send_file_stat(fd, &status, file->d_name);
        if(send_status < 0)
            return send_status;
    }
    return 0;
}

void read_request(int fd, char* buf) {
    int p = 0;
    while (1) {
        int n = read(fd, buf + p, BUFFER_SIZE - 1 - p);
        if (n < 0) {
            close(fd);
            return;
        } 
        else if (n == 0) {
            break;
        } 
        else {
            p += n;
            if (p >= 2 && buf[p - 2] == '\r' && buf[p - 1] == '\n') {
                break;
            }
        }
    }   
    buf[p - 2] = '\0';
}
void parse_request(char* buf, char* verb, char* param) {
    int len = strlen(buf);
    int read_verb = 1;
    int v_i = 0, p_i = 0;
    for(int i = 0; i < len; i++) {
        if(buf[i] == ' ') {
            read_verb = 0;
            continue;
        }
        if(read_verb) {
            verb[v_i] = buf[i];
            v_i++;
        }
        else {
            param[p_i] = buf[i];
            p_i++;
        }
    }
    verb[v_i] = '\0';
    param[p_i] = '\0';
}

void parse_mix_address(char* mix_address, char* ip, int* port) {
    //mixed address: h1,h2,h3,h4,p1,p2

    //port: p1*256+p2
    char* last_split = strrchr(mix_address, ',');
    *port = atoi(last_split + 1);   //p2
    *last_split = '\0';
    last_split = strrchr(mix_address, ',');
    *port += 256 * atoi(last_split + 1);    //256 * p1
    *last_split = '\0';

    //ip address: h1.h2.h3.h4 
    int len = strlen(mix_address);
    for(int i = 0; i < len; ++i)
        ip[i] = (mix_address[i] == ',' ? '.' : mix_address[i]);
}

void get_local_ip(int fd, char* ip) {
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    getsockname(fd, (struct sockaddr*) &name, &namelen);
    char buffer[32];
    inet_ntop(AF_INET, &name.sin_addr, buffer, sizeof(buffer));
    strcpy(ip, buffer);
}

void convert_path_format(char* source, char* target) {
    int len = strlen(source);
    int i, j;
    for(i = 0, j = 0; i < len; ++i, ++j) {
        if(source[i] == '\"') {
            target[j] = '\"';
            j++;
            target[j] = '\"';
        }
        else if(source[i] == 12)
            target[j] = '\0';
        else
            target[j] = source[i];
    }
    target[j] = '\0';
}

void get_absolute_directory(char* origin, char* target, char* current_root) {
    //TODO: handle ".." in path
    if(strlen(origin) == 0) {
        strcpy(target, current_root);
    }
    else if(origin[0] == '/') {
        strcpy(target, origin);
    }
    else {
        strcpy(target, current_root);
        strcat(target, "/");
        strcat(target, origin);
    }
}

int validate_user(char* username, char* password) {
    //TODO: validate through a user table
    return !strcmp(username, "anonymous") && strlen(password) > 0;
}

struct client_info* create_client_info(int fd, char* root, int epfd) {
    struct client_info* list = client_info_list;
    for(int i = 0; i < MAX_EPOLL; ++i) {
        if(!list[i].in_use) {
            list[i].in_use = 1;
            list[i].epfd = epfd;
            list[i].connfd = fd;
            list[i].has_username = 0;
            list[i].has_password = 0;
            list[i].has_login = 0;
            list[i].mode = NO;
            list[i].PASV_listenfd = -1;
            strcpy(list[i].root, root);
            return &list[i];
        }
    }
    return NULL;
}

struct client_info* get_client_info(int fd) {
    struct client_info* list = client_info_list;
    for(int i = 0; i < MAX_EPOLL; ++i) {
        if(list[i].in_use && list[i].connfd == fd) {
            list[i].connfd = fd;
            return &list[i];
        }
    }
    return NULL;
}

int close_client_info(int fd) {
    struct client_info* list = client_info_list;
    for(int i = 0; i < MAX_EPOLL; ++i) {
        if(list[i].in_use && list[i].connfd == fd) {
            list[i].in_use = 0;
            return 0;
        }
    }
    return -1;
}

void add_epoll_event(int epfd, int fd, int state) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = state;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

void delete_epoll_event(int epfd, int fd, int state) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = state;
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
}