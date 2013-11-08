#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#define TCPCLIENT_ENABLE_DEBUG_MSG        1

enum TCP_SHUTDOWN_OPT
{
    shudn_write_side,
    shudn_read_side
};



class TCPClient
{
public:
    TCPClient();
    ~TCPClient();

    static char*    errmsg(int system_error_code);

    void    shutdown_conn(TCP_SHUTDOWN_OPT opt=shudn_write_side);

    void    close();

    bool    connected()const ;

    // set local address for socket,it optional
    // @para ipv4 can be NULL
    // @para port can be 0
    void     set_local_address(const char* ipv4,unsigned short port);

    // get local address
    // return 0 when success
    int      get_local_address(char* ip,unsigned short* port,int* system_error_code);

    // connect to server
    // @return
    // =0 connecting successful
    //
    int     connect_to(const char* ipv4,unsigned short port,unsigned int timeout_ms,int* system_error_code);
    // write data
    // @return
    // >0 bytes has written
    // =0 need try again (no data has written and no error occurred)
    // <0 error occoured
    int     write_some(const void* data,int bytes,unsigned int timeout_ms,int* system_error_code);

    // write data
    // @return
    // >0 bytes has written
    // =0 need try again (no data has written and no error occurred)
    // <0 error occoured
    int     write_bytes(const void* data,int bytes,unsigned int timeout_ms,int* system_error_code);

    // read data
    // @return
    // >0 bytes has read
    // =0 connection closed by remote
    // <0 error occoured(includes timeout error)
    int     read_some(void* buff,int bytes,unsigned int timeout_ms,int* system_error_code);

    // read data
    // @return
    // >0 bytes has read
    // =0 connection closed by remote
    // <0 error occoured(includes timeout error)
    int     read_bytes(void* buff,int bytes,unsigned int timeout_ms,int* system_error_code);
private:
    int     bind_local_addr(int* system_error_code);
private:
    bool    connected_;
    int socket_fd_;
    char        local_ip_[32];
    unsigned short local_port_;

};

#endif // TCPCLIENT_H
