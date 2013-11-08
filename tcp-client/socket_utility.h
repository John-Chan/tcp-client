#ifndef SOCKET_UTILITY_H
#define SOCKET_UTILITY_H



// return -1 when fail
// return >0 ,the FD
int     tcp_create_socket(int* system_error_code);


bool     set_blocking(int fd, bool blocking,int* system_error_code);


bool     wait_for_connect(int fd,int timeout_ms,int* system_error_code);

// return 0 error occoured,check system_error_code to see the errorno
// return 1 readable
// return 2 writeable
// return 3 readable and writeable
// @note if both wait_read and wait_write are false,renturn 0,and  system_error_code set to 0
int      wait_for_rw(int fd, int timeout_ms, bool wait_read,bool wait_write,int *system_error_code);

// make binary socket_addr
// @para ip can be NULL
// @para port can be 0
// return false when fail(the address is invalid)
bool    ipv4_make_addr(const char* ip,unsigned short port,void* out);


bool     bind_addr(int fd,const void* buff_sockaddr_in,int buff_bytes,int* system_error_code);

// get last error from a socket
bool    get_last_err_from_fd(int fd,int* error_code);

bool    get_local_addr_from_fd(int fd,char* buff_for_ip,unsigned short* port, int* error_code);

#endif // SOCKET_UTILITY_H
