#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./include/TcpServer.h"
#include "./include/Log.h"

int main(int argc, char* argv[]){
#if 0
    if(argc < 3){
        printf("./a.out pport path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    chdir(argv[2]);
#else
    unsigned short port = 10000;
    chdir("/usr/ebor/");
#endif
    // 启动服务器
    TcpServer* server = new TcpServer(port, 4);
    Debug("服务器程序启动了...");
    server->run();
    Debug("怎么挂了");
    return 0;
}