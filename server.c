#include "server.h"

void send_response(int fd, char* sentence) {
	int p = 0;
	int len = strlen(sentence);
	while (p < len) {
		int n = write(fd, sentence + p, len - p);
		if (n < 0) {
			printf("Error write(): %s(%d)\n", strerror(errno), errno);
			return;
	 	} 
	 	else {
			p += n;
		}			
	}	
}

void read_command(int fd, char* buf) {
	int p = 0;
	while (1) {
		int n = read(fd, buf + p, 8191 - p);
		if (n < 0) {
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
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

void serve_client(int fd) {

	struct client_info info = {
		.has_username = 0,
		.has_password = 0,
	};
	send_response(fd, greeting);
	while(1) {
		//read
		char buffer[8192];
		read_command(fd, buffer);

		//parse
		char verb[8192], parameter[8192];
		parse_request(buffer, verb, parameter);

		//handle
		if(!strcmp(verb, "USER")) {
			if(!strcmp(parameter, "anonymous")) {
				info.has_username = 1;
				send_response(fd, pass_required);
			}
			else
				send_response(fd, not_anonymous);
		}
		else if(!strcmp(verb, "PASS")) {
			if(info.has_username) {
				info.has_password = 1;
				send_response(fd, login_success);
			}
			else {
				send_response(fd, login_fail);
			}
		}
		else if(!strcmp(verb, "QUIT")) {
			send_response(fd, quit);
			break;
		}
		else {
			send_response(fd, resolve_fail);
		}
	}
}

int main(int argc, char **argv) {

	//init
	int listenfd, connfd;
	struct sockaddr_in addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(21);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	printf("FTP server started @localhost:%d\n", 21);

	while (1) {
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		serve_client(connfd);
		close(connfd);
	}
	close(listenfd);
}
