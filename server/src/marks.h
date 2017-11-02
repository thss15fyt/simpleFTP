#pragma once

/*marks*/
//general
#define greeting  "220 FTP server ready.\r\n"
#define login_required  "530 Please login first.\r\n"
#define resolve_fail  "502 Can't resolve request.\r\n"

//USER
#define not_anonymous  "530 Only anonymous user is supported.\r\n"
#define password_required  "331 Guest login ok, send your complete e-mail address as password.\r\n"

//PASS
#define username_required  "530 Need username before password.\r\n"
#define login_fail  "530 Wrong password.\r\n"
#define login_success  "230 Guest login ok.\r\n"

//SYST
#define system_info  "215 UNIX Type: L8\r\n"

//TYPE
#define type_set_i  "200 Type set to I.\r\n"
#define type_not_support  "502 Type is not supported.\r\n"

//PORT
#define port_success  "200 PORT command successful.\r\n"

//PASV
#define pasv_success   "227 Entering Passive Mode (%s,%d,%d)\r\n"

//RETR & STOR & LIST
#define mode_not_set  "503 Mode is not set.\r\n"
#define connect_fail  "425 No TCP connection was established.\r\n"
#define connect_break  "426 TCP connection was established but then broken by the client or by network failure.\r\n"

//RETR
#define send_begin  "150 Opening BINARY mode data connection for %s (%d bytes).\r\n"
#define send_over  "226 Transfer complete.\r\n"
#define get_file_fail  "551 Can't find file.\r\n"
#define read_file_fail  "451 Server has trouble reading the file from disk.\r\n"

//STOR
#define recv_begin  "150 Opening BINARY mode data connection for %s.\r\n"
#define recv_over  "226 Transfer complete.\r\n"
#define create_file_fail  "551 Can't create file.\r\n"
#define write_file_fail  "451 Server has trouble saving the file from disk.\r\n"

//MKD
#define mkdir_success "257 Create directory \"%s\".\r\n"
#define mkdir_fail "550 Failed to create directory.\r\n"

//CWD
#define cwd_success "250 Okay.\r\n"
#define cwd_fail "550 %s: No such file or directory.\r\n"

//LIST
#define file_format "-rw-r--r-- 1 %-13s %-13s %13d %3s%3d %02d:%02d %s\r\n"
#define directory_format "drwxr-xr-x 1 %-13s %-13s %13d %3s%3d %02d:%02d %s\r\n"
#define list_send_begin "150 Here comes the directory listing.\r\n"
#define list_send_over "226 Directory send OK.\r\n"
#define read_directory_fail "451 Server has trouble reading the directory from disk.\r\n"

//PWD
#define pwd_name_prefix "257 \"%s\" is the current directory.\r\n"

//RMD
#define rmdir_success "250 Remove directory \"%s\".\r\n"
#define rmdir_fail "550 Failed to remove directory.\r\n"

//QUIT ABOR
#define quit  "221 Goodbye.\r\n"
