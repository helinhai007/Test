#include <stdio.h>
#include<unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<time.h>
#include <stdarg.h>
#include <iostream>
#include <cstdlib>

 
#define BUF_SIZE 1024

 using namespace std;
int main(){

    //先检查文件是否存在

    //char *filename = "123.txt"; //文件名
    FILE *fp = fopen("123.txt", "wb"); //以二进制方式打开文件

    if(fp == NULL){
    printf("Cannot open file, press any key to exit!\n");
    exit(0);

    }


    int servSock = socket(AF_INET, SOCK_STREAM, 0);

    
    sockaddr_in sockAddr;

    memset(&sockAddr, 0, sizeof(sockAddr));

    sockAddr.sin_family = AF_INET;

    sockAddr.sin_addr.s_addr=htonl(INADDR_ANY);

    sockAddr.sin_port = htons(1234);

    if(bind(servSock, (const struct sockaddr*)&sockAddr, sizeof(sockAddr)) != 0)
    {
        cout << "bind failed" << endl;
        return -1;
    }
    cout << "bind sucess" << endl;

    if(listen(servSock, 20) != 0)
    {
        cout << "listen failed" << endl;
        return -1;
    }
    cout << "listen sucess" << endl;
    
    int socklen;
    struct sockaddr_in clientaddr;  //客户端的地址信息

    int clntSock = accept(servSock, (struct sockaddr *)&clientaddr, (socklen_t*)&socklen);

    cout << "accept sucess" << endl;
    
    //循环发送数据，直到文件结尾

    // char buffer[BUF_SIZE] = {0}; //缓冲区

    // int nCount;

    // while( (nCount = fread(buffer, 1, BUF_SIZE, fp)) > 0 ){

    // send(clntSock, buffer, nCount, 0);

    // }
    // cout << "send sucess" << endl;
    
    // shutdown(clntSock, 1); //文件读取完毕，断开输出流，向客户端发送FIN包

    // recv(clntSock, buffer, BUF_SIZE, 0); //阻塞，等待客户端接收完毕

    //循环接收数据，直到文件传输完毕

  char buffer[BUF_SIZE] = {0}; //文件缓冲区

  int nCount;

  while( (nCount = recv(clntSock, buffer, BUF_SIZE, 0)) > 0 ){

  fwrite(buffer, nCount, 1, fp);

  }

  cout << "recv sucess" << endl;

    
    fclose(fp);

    close(clntSock);

    close(servSock);

    cout << "send end" << endl;

    return 0;

}

