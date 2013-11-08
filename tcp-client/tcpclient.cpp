#include "tcpclient.h"

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <stdarg.h>        /* for va_start,va_list */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>    /* According to POSIX.1-2001 */
#include <sys/time.h>      /* According to earlier standards */

#include "socket_utility.h"

#define     INVALID_FD      (-1)
#define     SYS_CALL_FAIL   (-1)

void dbg_msg(const char* format, ...)
{
#if TCPCLIENT_ENABLE_DEBUG_MSG
    va_list args;
    va_start(args, format);

    char line[512] = {0};
    vsprintf(line, format, args);

    va_end(args);

    printf("%s\r\n",line);
    //printf(line);
#endif//TCPCLIENT_ENABLE_DEBUG_MSG

}


TCPClient::TCPClient()
    :socket_fd_(INVALID_FD),
      connected_(false)
{
    int system_error_code=0;
    ::memset(local_ip_,0x0,sizeof(local_ip_));
    local_port_=0;
    socket_fd_=tcp_create_socket(&system_error_code);
    if(socket_fd_ == INVALID_FD){
        dbg_msg("create socket fail,errno=%d,error msg=%s",system_error_code,::strerror(system_error_code));
    }else{
        if( !set_blocking(socket_fd_,false,&system_error_code) ){
            dbg_msg("set nonblocking fail,errno=%d,error msg=%s",system_error_code,::strerror(system_error_code));
        }
    }
}
TCPClient::~TCPClient()
{
    shutdown_conn(shudn_write_side);
    close();
}

char*    TCPClient::errmsg(int system_error_code)
{
    return ::strerror(system_error_code);
}

void    TCPClient::shutdown_conn(TCP_SHUTDOWN_OPT opt)
{
    int how=(opt == shudn_read_side)?SHUT_RD: SHUT_WR;
    ::shutdown(socket_fd_,how);
}
void    TCPClient::close()
{
    ::close(socket_fd_);
    socket_fd_=INVALID_FD;
}


void     TCPClient::set_local_address(const char* ipv4,unsigned short port)
{
    if(!connected_ ){
        if(ipv4)
            ::strncpy(local_ip_,ipv4,sizeof(local_ip_)-1);
        else
            local_ip_[0]='\0';
        local_port_=port;
    }
}
bool    TCPClient::connected()const
{
    return connected_;
}

int     TCPClient::get_local_address(char* ip, unsigned short* port, int *system_error_code)
{
    return get_local_addr_from_fd(socket_fd_,ip,port,system_error_code)? 0:SYS_CALL_FAIL;
}

int     TCPClient::bind_local_addr(int* system_error_code)
{
    //dbg_msg("start bind local_addr");
    struct sockaddr_in local_addr;
    const char* bind_local_ip= (::strlen(local_ip_)>0)?local_ip_:NULL;
    if( bind_local_ip!=NULL || local_port_ != 0){
        ipv4_make_addr(bind_local_ip,local_port_,&local_addr);
        if(!bind_addr(socket_fd_,&local_addr,sizeof(local_addr),system_error_code)){
            dbg_msg("bind_addr fail,errno=%d,error msg=%s",*system_error_code,::strerror(*system_error_code));
            return SYS_CALL_FAIL;
        }
    }else{
        //dbg_msg("no need bind local_addr");
    }
    return 0;
}

int     TCPClient::connect_to(const char* ipv4, unsigned short port, unsigned int timeout_ms, int *system_error_code)
{

    //dbg_msg("ipv4_make_addr :%s %d",ipv4,port);
    struct sockaddr_in remote_addr;
    ipv4_make_addr(ipv4,port,&remote_addr);

    if(0 != bind_local_addr(system_error_code))
        return SYS_CALL_FAIL;

    dbg_msg("start connect :%s %d",ipv4,port);
    int stat_code=::connect(socket_fd_,(sockaddr*)(&remote_addr),sizeof(remote_addr));
    if (stat_code == SYS_CALL_FAIL) {
        if (errno != EINPROGRESS) {
            dbg_msg("connect fail,errno=%d,error msg=%s",errno,::strerror(errno));
            return SYS_CALL_FAIL;
        }else{
            if( !wait_for_connect(socket_fd_,timeout_ms,system_error_code)){
                dbg_msg("wait_for_connect fail,errno=%d,error msg=%s",*system_error_code,::strerror(*system_error_code));
                return SYS_CALL_FAIL;
            }else{
                connected_=true;
                return 0;
            }
        }
    }
    connected_=true;
    return 0;
}
int     TCPClient::write_some(const void* data, int bytes, unsigned int timeout_ms, int *system_error_code)
{
    if(bytes<=0){
        dbg_msg("bad call:attempt to write %d bytes",bytes);
        return SYS_CALL_FAIL;
    }

    // wait_for_rw return 0 error occoured,check system_error_code to see the errorno
    // wait_for_rw return 1 readable
    // wait_for_rw return 2 writeable
    // wait_for_rw return 3 readable and writeable
    int stat_code=wait_for_rw(socket_fd_,timeout_ms,false,true,system_error_code);

    if( 2 == stat_code || 3== stat_code){
        stat_code=::write(socket_fd_,data,bytes);
        if( SYS_CALL_FAIL == stat_code){
            *system_error_code=errno;
            dbg_msg("write fail,errno=%d,error msg=%s",*system_error_code,::strerror(*system_error_code));
        }
        return stat_code;
    }else{
        dbg_msg("wait_for_rw return %d,errno=%d,error msg=%s",stat_code,*system_error_code,::strerror(*system_error_code));

        if(  0 == stat_code && (*system_error_code)== ETIME/*ETIMEDOUT*/){
            dbg_msg("write opeatrion is not ready,timeout(milliseconds):%u,the send buffer may full,need try it again latter",timeout_ms);
            return 0;
        }
        return SYS_CALL_FAIL;
    }
    return stat_code;
}
int     TCPClient::read_some(void* buff, int bytes, unsigned int timeout_ms, int *system_error_code)
{
    if(bytes<=0){
        dbg_msg("bad call:attempt to read %d bytes",bytes);
        return SYS_CALL_FAIL;
    }
    // wait_for_rw return 0 error occoured,check system_error_code to see the errorno
    // wait_for_rw return 1 readable
    // wait_for_rw return 2 writeable
    // wait_for_rw return 3 readable and writeable
    int stat_code=wait_for_rw(socket_fd_,timeout_ms,true,false,system_error_code);

    if(1== stat_code || 3== stat_code ){
        stat_code=::read(socket_fd_,buff,bytes);
        if( SYS_CALL_FAIL == stat_code){
            *system_error_code=errno;
            dbg_msg("read fail,errno=%d,error msg=%s",*system_error_code,::strerror(*system_error_code));
        }
        return stat_code;
    }else{
        dbg_msg("wait_for_rw return %d,errno=%d,error msg=%s",stat_code,*system_error_code,::strerror(*system_error_code));

        if(  0 == stat_code && (*system_error_code)== ETIME/*ETIMEDOUT*/){
            dbg_msg("read operation is not ready,timeout(milliseconds):%u",timeout_ms);
        }
        return SYS_CALL_FAIL;
    }
}

int     TCPClient::write_bytes(const void* data, int bytes, unsigned int timeout_ms, int *system_error_code)
{
    int have_write=0;
    int stat_code=0;
    const char* buff_ptr=(const char*)data;
    do{
        stat_code=write_some(buff_ptr+have_write,bytes-have_write,timeout_ms,system_error_code);
        if(0 == stat_code ){
            // need try again
            return stat_code;
        }else if(0 > stat_code){
            // error
            dbg_msg("error occoured,data write %d",have_write);
            return stat_code;
        }else{
            // ok ,get some data
            have_write+=stat_code;
            if(have_write>=bytes){
                return have_write;
            }
        }
    }while(1);
    return SYS_CALL_FAIL;
}
int     TCPClient::read_bytes(void* buff, int bytes, unsigned int timeout_ms, int *system_error_code)
{
    static const int kReadBytesOnce=sizeof(void*) * 32;
    int have_read=0;
    int stat_code=0;
    char* buff_ptr=(char*)buff;
    do{
        stat_code=read_some(buff_ptr+have_read,kReadBytesOnce,timeout_ms,system_error_code);

        if(0 == stat_code ){
            // EOF:connection closed by remote
            dbg_msg("read EOF:connection closed by remote,data readed %d",have_read);
            return stat_code;
        }else if(0 > stat_code){
            // error
            dbg_msg("error occoured,data readed %d",have_read);
            return stat_code;
        }else{
            // ok ,get some data
            have_read+=stat_code;
            if(have_read>=bytes){
                return have_read;
            }
        }
    }while(1);
    return SYS_CALL_FAIL;
}
