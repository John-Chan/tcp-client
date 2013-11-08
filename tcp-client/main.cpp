#include <iostream>

using namespace std;
#include "tcpclient.h"

const char* remote_ip="192.168.0.180";
unsigned short remote_port=34000;
int system_error_code=0;

void    test_1();
void    test_2();
int main()
{
    //test_1();
    test_2();
    return 0;
}

void    test_1()
{
    char my_ip[32]={0};
    unsigned short my_port=0;
    TCPClient cli;
    cli.set_local_address(NULL,0);
    //cli.set_local_address("192.168.0.129",65530);

    cli.get_local_address(my_ip,&my_port,&system_error_code);
    cout << "errorno:" << system_error_code << endl;
    cout << "local address(before connect):" << my_ip<< " " << my_port<< endl;



    if(0==cli.connect_to(remote_ip,remote_port,1000,&system_error_code)){
        cout << "connect successful:" << remote_ip<< " "<< remote_port << endl;
        cli.get_local_address(my_ip,&my_port,&system_error_code);
        cout << "errorno:" << system_error_code << endl;
        cout << "local address(after connect):" << my_ip<< " " << my_port<< endl;

        int io_stat_code=0;
        char recv_buff[512]={0};
        io_stat_code=cli.write_bytes("abcd",4,1000,&system_error_code);
        cout << "operation result for  write_bytes() call:" << io_stat_code
             <<",errorno : "<<system_error_code <<",msg="<<TCPClient::errmsg(system_error_code)
             << endl;
        io_stat_code=cli.read_bytes(recv_buff,sizeof(recv_buff),1000,&system_error_code);
        cout << "operation result for  read_bytes() call:" << io_stat_code
             <<",errorno : "<<system_error_code <<",msg="<<TCPClient::errmsg(system_error_code)
             << endl;
        cout << "recv:" << recv_buff << endl;
    }else{
        cout << "connect fail:" << remote_ip<< " "<< remote_port << endl;
        cout << "ret:" << system_error_code << endl;
    }
}

void    test_2()
{
    char my_ip[32]={0};
    unsigned short my_port=0;
    TCPClient cli;
    cli.set_local_address(NULL,0);

    cli.get_local_address(my_ip,&my_port,&system_error_code);
    cout << "errorno:" << system_error_code << endl;
    cout << "local address(before connect):" << my_ip<< " " << my_port<< endl;


    int loop=10000;
    if(0==cli.connect_to(remote_ip,remote_port,1000,&system_error_code)){
        cout << "connect successful:" << remote_ip<< " "<< remote_port << endl;
        cli.get_local_address(my_ip,&my_port,&system_error_code);
        cout << "errorno:" << system_error_code << endl;
        cout << "local address(after connect):" << my_ip<< " " << my_port<< endl;

        int io_stat_code=0;
        char recv_buff[512]={0};

        int run_count=0;
        while(run_count++ <loop){
            cout << "loop:" << run_count << endl;
            io_stat_code=cli.write_some("abcd",4,1000,&system_error_code);
            if(io_stat_code<=0){
                cout << "operation result for  write_some() call:" << io_stat_code
                     <<",errorno : "<<system_error_code <<",msg="<<TCPClient::errmsg(system_error_code)
                     << endl;
                break;
            }
            io_stat_code=cli.read_some(recv_buff,sizeof(recv_buff),1000,&system_error_code);
            if(io_stat_code<=0){
                cout << "operation result for  read_some() call:" << io_stat_code
                     <<",errorno : "<<system_error_code <<",msg="<<TCPClient::errmsg(system_error_code)
                     << endl;
                break;
            }
            //cout << "recv:" << recv_buff << endl;
        }
    }else{
        cout << "connect fail:" << remote_ip<< " "<< remote_port << endl;
        cout << "ret:" << system_error_code << endl;
    }
}
