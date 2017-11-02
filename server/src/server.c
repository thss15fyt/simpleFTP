#include "server.h"

struct client_info* client_info_list = NULL;
int listenfd = -1;

void sig_int() {
	close(listenfd);
	exit(0);
}

int main(int argc, char **argv) {

	signal(SIGINT, &sig_int);
	
	//port and root
	int port = DEFAULT_PORT;
	char root[BUFFER_SIZE];
	strcpy(root, DEFAULT_ROOT);
	for(int i = 0; i < argc; ++i) {
		if(!strcmp(argv[i], "-port")) {
			port = atoi(argv[i+1]);
		}
		else if(!strcmp(argv[i], "-root")) {
			strcpy(root, argv[i+1]);
		}
	}

	int listenfd, connfd;
	if((listenfd = listen_port(port)) == -1)
		return 1;
	printf("FTP server started @localhost:%d root:\"%s\"\n", port, root);

	struct epoll_event events[MAX_CLIENT];
	int epfd = epoll_create(MAX_EPOLL);
	add_epoll_event(epfd, listenfd, EPOLLIN);

	client_info_list = (struct client_info*)malloc(MAX_EPOLL * sizeof(struct client_info));
	for(int i = 0; i < MAX_EPOLL; ++i)
		client_info_list[i].in_use = 0;

	while (1) {
		int nfds = epoll_wait(epfd, events, MAX_CLIENT, 500);
		for(int i = 0; i < nfds; ++i) {
        	int fd  = events[i].data.fd;
        	struct client_info* info;
            if(fd == listenfd) {
            	if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
					continue;
				}
				add_epoll_event(epfd, connfd, EPOLLIN);
                create_client_info(connfd, root, epfd);
                send_response(connfd, greeting);
            } 
            else if(events[i].events & EPOLLIN) {
            	if((info = is_data_connect(fd)) != NULL) {
            		recv_file(info);
            	}
            	else if((info = is_command_connect(fd)) != NULL){
            		serve_client(info, fd);
            	}
            }
            else if(events[i].events & EPOLLOUT) {
            	if((info = is_data_connect(fd)) != NULL) {
            		send_file(info);
            	}
            }
        }
    }		
	close(listenfd);

	return 0;
}

void serve_client(struct client_info* info, int fd) {
	
	//read
	char buffer[BUFFER_SIZE];
	read_request(fd, buffer);

	//parse
	char verb[BUFFER_SIZE], parameter[BUFFER_SIZE];
	parse_request(buffer, verb, parameter);

	//handle
	if(!strcmp(verb, "USER"))
		command_USER(fd, parameter, info);
	else if(!strcmp(verb, "PASS"))
		command_PASS(fd, parameter, info);
	else if(!strcmp(verb, "SYST"))
		command_SYST(fd, parameter, info);
	else if(!strcmp(verb, "TYPE"))
		command_TYPE(fd, parameter, info);
	else if(!strcmp(verb, "PORT"))
		command_PORT(fd, parameter, info);
	else if(!strcmp(verb, "PASV"))
		command_PASV(fd, parameter, info);
	else if(!strcmp(verb, "RETR")) 
		command_RETR(fd, parameter, info);
	else if(!strcmp(verb, "STOR"))
		command_STOR(fd, parameter, info);
	else if(!strcmp(verb, "MKD"))
		command_MKD(fd, parameter, info);
	else if(!strcmp(verb, "CWD"))
		command_CWD(fd, parameter, info);
	else if(!strcmp(verb, "LIST"))
		command_LIST(fd, parameter, info);
	else if(!strcmp(verb, "RMD"))
		command_RMD(fd, parameter, info);
	else if(!strcmp(verb, "PWD"))
		command_PWD(fd, parameter, info);
	else if(!strcmp(verb, "QUIT") || !strcmp(verb, "ABOR"))
		command_QUIT(fd, parameter, info);
	else
		command_UNKNOWN(fd, parameter, info);

}