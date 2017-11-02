#include "server.h"

/* command handle functions */

int command_USER(int fd, char* parameter, struct client_info* info) {
    strcpy(info->username, parameter);
    info->has_username = 1;
    send_response(fd, password_required);

    return 0;
}

int command_PASS(int fd, char* parameter, struct client_info* info) {
    if(info->has_username) {
        if(validate_user(info->username, parameter)) {
            info->has_password = 1;
            info->has_login = 1;
            send_response(fd, login_success);
        }
        else
            send_response(fd, login_fail);
    }
    else {
        send_response(fd, username_required);
    }

    return 0;
}

int command_SYST(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }
    send_response(fd, system_info);

    return 0;
}

int command_TYPE(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }
    if(!strcmp(parameter, "I")) 
        send_response(fd, type_set_i);
    else
        send_response(fd, type_not_support);

    return 0;
}

int command_PORT(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }
    parse_mix_address(parameter, info->PORT_ip, &info->PORT_port);
    close(info->PASV_listenfd);
    info->mode = PORT;
    send_response(fd, port_success);

    return 0;
}

int command_PASV(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }
    info->mode = PASV;
    //close listening socket
    close(info->PASV_listenfd);
    //get random avaliable port
    srand(time(NULL));
    while(1) {
        info->PASV_port = rand() % (MAX_PORT - MIN_PORT + 1) + MIN_PORT;
        if((info->PASV_listenfd = listen_port(info->PASV_port)) != -1)
            break;
    }
    //get local ip and replace '.'
    char ip[32];
    get_local_ip(info->connfd, ip);
    int ip_len = strlen(ip);
    for(int i = 0; i < ip_len; ++i)
        ip[i] = (ip[i] == '.' ? ',' : ip[i]);
    //send response
    char response[BUFFER_SIZE];
    sprintf(response, pasv_success,
        ip, info->PASV_port / 256, info->PASV_port % 256);
    send_response(fd, response);

    return 0;
}

int command_RETR(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }
    if(info->mode != NO) {
        char filename[BUFFER_SIZE];
        strcpy(filename, info->root);
        strcat(filename, "/");
        strcat(filename, parameter);
        struct stat status;
        if((info->file = fopen(filename, "rb")) != NULL && stat(filename, &status) == 0) {
            //connect
            if(info->mode == PASV) {
                info->data_connfd = accept(info->PASV_listenfd, NULL, NULL);
            }
            else if(info->mode == PORT) {
                info->data_connfd = connect_port(info->PORT_ip, info->PORT_port);
            }
            //send
            if (info->data_connfd != -1) {
                //send begin response
                char response[BUFFER_SIZE];
                sprintf(response, send_begin, parameter, (int)status.st_size);
                send_response(fd, response);
                //send file
                int send_status;
                if((send_status = send_file(info->data_connfd, info->file)) == -1) {    //read from file fail
                    send_response(fd, read_file_fail);
                }
                else if(send_status == -2){     //write to socket fail
                    send_response(fd, connect_break);
                }
                else {  //send success
                    send_response(fd, send_over);
                }
                if(info->mode == PASV)
                    close(info->PASV_listenfd);
                close(info->data_connfd);
            }
            else {  //connect or accept fail
                send_response(fd, connect_fail);
            }
            fclose(info->file);
        }
        else {  //find and open file fail
            send_response(fd, get_file_fail);
        }
    }
    else {
        send_response(fd, mode_not_set);
    }

    return 0;
}

int command_STOR(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }
    if(info->mode != NO) {
        char filename[BUFFER_SIZE];
        strcpy(filename, info->root);
        strcat(filename, "/");
        strcat(filename, parameter);
        if((info->file = fopen(filename, "wb")) != NULL) {
            //connect
            if(info->mode == PASV) {
                info->data_connfd = accept(info->PASV_listenfd, NULL, NULL);
            }
            else if(info->mode == PORT) {
                info->data_connfd = connect_port(info->PORT_ip, info->PORT_port);
            }
            //receive
            if (info->data_connfd != -1) {
                //receive begin response
                char response[BUFFER_SIZE];
                sprintf(response, recv_begin, parameter);
                send_response(fd, response);
                //receive file
                int send_status;
                if((send_status = recv_file(info->data_connfd, info->file)) == -1) {    //read from socket fail
                    send_response(fd, connect_break);
                }
                else if(send_status == -2){     //write to file fail
                    send_response(fd, write_file_fail);
                }
                else {   //receive success
                    send_response(fd, recv_over);
                }
                if(info->mode == PASV)
                    close(info->PASV_listenfd);
                close(info->data_connfd);
            }
            else {  //connect or accept fail
                send_response(fd, connect_fail);
            }
            fclose(info->file);
        }
        else {  //create file fail
            send_response(fd, create_file_fail);
        }
    }
    else {
        send_response(fd, mode_not_set);
    }

    return 0;
}

int command_MKD(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }
    
    char directory[BUFFER_SIZE];
    get_absolute_directory(parameter, directory, info->root);

    if(mkdir(directory, S_IRWXU) != 0) {
        send_response(fd, mkdir_fail);
    }
    else {
        char prefix[BUFFER_SIZE];
        convert_path_format(directory, prefix);
        char response[BUFFER_SIZE];
        sprintf(response, mkdir_success, prefix);
        send_response(fd, response);
    }

    return 0;
}

int command_CWD(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }

    char directory[BUFFER_SIZE];
    get_absolute_directory(parameter, directory, info->root);

    DIR* dir;
    if((dir = opendir(directory)) != NULL) {
        strcpy(info->root, directory);
        send_response(fd, cwd_success);
        closedir(dir);
    }
    else {
        char response[BUFFER_SIZE];
        sprintf(response, cwd_fail, directory);
        send_response(fd, response);
    }

    return 0;
}

int command_LIST(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }

    if(info->mode != NO) {
        char directory[BUFFER_SIZE];
        get_absolute_directory(parameter, directory, info->root);
        struct stat status;
        DIR* dir = opendir(directory);
        if(stat(directory, &status) == 0 || dir != NULL) {
            //connect
            if(info->mode == PASV) {
                info->data_connfd = accept(info->PASV_listenfd, NULL, NULL);
            }
            else if(info->mode == PORT) {
                info->data_connfd = connect_port(info->PORT_ip, info->PORT_port);
            }
            //send
            if (info->data_connfd != -1) {
                //send begin response
                char response[BUFFER_SIZE];
                send_response(fd, list_send_begin);
                //send list
                int send_status;
                if(dir != NULL) {
                    send_status = send_directory_content(info->data_connfd, dir, directory);
                    closedir(dir);
                }
                else {
                    send_status = send_file_stat(info->data_connfd, &status, basename(directory));
                }

                if(send_status == -1)  //read from file fail
                    send_response(fd, read_directory_fail);
                else if(send_status == -2)    //write to socket fail
                    send_response(fd, connect_break);
                else  //send success
                    send_response(fd, list_send_over);

                if(info->mode == PASV)
                    close(info->PASV_listenfd);
                close(info->data_connfd);
            }
            else {  //connect or accept fail
                send_response(fd, connect_fail);
            }
        }
        else {  //find and open file fail
            send_response(fd, read_directory_fail);
        }
    }
    else {
        send_response(fd, mode_not_set);
    }

    return 0;
}

int command_RMD(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }

    char directory[BUFFER_SIZE];
    get_absolute_directory(parameter, directory, info->root);

    if(rmdir(directory) != 0) {
        send_response(fd, rmdir_fail);
    }
    else {
        char response[BUFFER_SIZE];
        sprintf(response, rmdir_success, directory);
        send_response(fd, response);
    }

    return 0;
}

int command_PWD(int fd, char* parameter, struct client_info* info) {
    if(!info->has_login) {
        send_response(fd, login_required);
        return 0;
    }

    char prefix[BUFFER_SIZE];
    convert_path_format(info->root, prefix);

    char response[BUFFER_SIZE];
    sprintf(response, pwd_name_prefix, prefix);
    send_response(fd, response);

    return 0;
}

int command_QUIT(int fd, char* parameter, struct client_info* info) {
    if(info->PASV_listenfd > 0)
        close(info->PASV_listenfd);

    send_response(fd, quit);    // TODO: response more info
    close(fd);
    close_client_info(fd);
    delete_epoll_event(info->epfd, fd, EPOLLIN | EPOLLET);

    return 0;
}

int command_UNKNOWN(int fd, char* parameter, struct client_info* info) {
    send_response(fd, resolve_fail);

    return 0;
}


