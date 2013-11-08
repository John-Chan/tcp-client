#include "socket_utility.h"

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>    /* According to POSIX.1-2001 */
#include <sys/time.h>      /* According to earlier standards */
#include <errno.h>

int     tcp_create_socket(int *system_error_code)
{
    *system_error_code=0;
    int fd=::socket(AF_INET,SOCK_STREAM,0);
    if( -1 == fd){
        *system_error_code=errno;
        return -1;
    }
    return fd;
}

bool     set_blocking(int fd, bool blocking, int *system_error_code)
{
   *system_error_code=0;

   int flags = ::fcntl(fd, F_GETFL, 0);
   if (flags < 0) {
       *system_error_code=errno;
       return false;
   }
   flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
   if(::fcntl(fd, F_SETFL, flags) < 0){
       *system_error_code=errno;
       return false;
   }
   return true;

}
bool    get_last_err_from_fd(int fd,int* error_code)
{
    socklen_t len=(socklen_t)sizeof(int);
    if(0 == ::getsockopt(fd, SOL_SOCKET, SO_ERROR, error_code, &len)){
        return true;
    }else{
        return false;
    }

}
bool     get_local_addr_from_fd(int fd,char* buff_for_ip,unsigned short* port, int* error_code)
{
    struct sockaddr_in local_address;
    socklen_t len = (socklen_t)sizeof(local_address);
    if( -1 == getsockname(fd, ( sockaddr*)(&local_address), &len)){
        *error_code=errno;
        return false;
    }else{
        *port = ::ntohs(local_address.sin_port);
        ::strcpy(buff_for_ip,::inet_ntoa(local_address.sin_addr));
        return true;
    }
}

bool     wait_for_connect(int fd, int timeout_ms, int *system_error_code)
{
    return wait_for_rw(fd,timeout_ms,false,true,system_error_code)==2;
}

int     wait_for_rw(int fd, int timeout_ms, bool wait_read,bool wait_write,int *system_error_code)
{

    *system_error_code=0;

    if(!wait_read && !wait_write){
        return 0;
    }

    fd_set readfds,writefds,exceptfds;
    struct timeval tval;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    FD_ZERO(&exceptfds);
    FD_SET(fd, &exceptfds);

    tval.tv_sec = timeout_ms/1000;
    tval.tv_usec = (timeout_ms%1000)*1000;

    fd_set* rd_set_ptr=wait_read?(&readfds):(NULL);
    fd_set* wt_set_ptr=wait_write?(&writefds):(NULL);
    fd_set* ex_set_ptr=&exceptfds;
    int stat_code = select(fd+1, rd_set_ptr, wt_set_ptr, ex_set_ptr, &tval);

    // the select call fail
    if (stat_code == -1){
        *system_error_code=errno;
        return 0;
    }

    // timeout
    if (stat_code == 0) {
        //*system_error_code=ETIMEDOUT;
        *system_error_code=ETIME;
        return 0;
    }

    // exception occured on fd
    if (FD_ISSET(fd, ex_set_ptr)){
        // FIXME: call get_last_err_from_fd fail?
        get_last_err_from_fd(fd,system_error_code);
        return 0;
    }

    stat_code=0;
    if (wait_read &&  FD_ISSET(fd, rd_set_ptr)){
        stat_code=1;
    }
    if (wait_write && FD_ISSET(fd, wt_set_ptr)){
        stat_code=(stat_code==1)?3:2;
    }

    return stat_code;
}

bool     bind_addr(int fd, const void* buff_sockaddr_in, int buff_bytes, int *system_error_code)
{
    *system_error_code=0;
    if( -1 == ::bind(fd,(const sockaddr*)buff_sockaddr_in,buff_bytes)){
        *system_error_code=errno;
        return false;
    }
    return true;
}

bool ipv4_make_addr(const char* ip,unsigned short port,void* out_sockaddr_in)
{
    struct sockaddr_in* p_addr=( struct sockaddr_in *)out_sockaddr_in;
    p_addr->sin_family= AF_INET;
    p_addr->sin_port=::htons(port);
    //::memset(p_addr->sin_zero,0x0,sizeof(p_addr->sin_zero));
    int ret=true;
    if(ip){
        ret=( 0 !=::inet_aton(ip,&(p_addr->sin_addr)) );
    }
    return ret;
}
