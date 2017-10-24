#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>


//client info
struct client_info {
    int has_username;
    int has_password;
};

//sentences
char* greeting = "220 FTP server ready.\r\n";
char* not_anonymous = "530 Only anonymous user is supported.\r\n";
char* pass_required = "331 Guest login ok, send your complete e-mail address as password.\r\n";
char* login_fail = "530 Wrong password.\r\n";
char* login_success = "230 Guest login ok.\r\n";
char* login_required = "";
char* quit = "221 Goodbye.\r\n";
char* resolve_fail = "502 Can't resolve request.\r\n";
