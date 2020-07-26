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
#include <string>
 
#define BUF_SIZE 1024

 using namespace std;
int main(){

  //先输入文件名，看文件是否能创建成功

  char filename[100] = {0}; //文件名

  cout << "Input filename to save: " << endl;

  //getline(cin, filename);
  cin >> filename;

  cout << filename << endl;

  FILE *fp = fopen(filename, "rb"); //以二进制方式打开（创建）文件

  if(fp == NULL){

  printf("Cannot open file, press any key to exit!\n");

  system("pause");

  exit(0);

  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);

  
  sockaddr_in sockAddr;

  memset(&sockAddr, 0, sizeof(sockAddr));

  sockAddr.sin_family = PF_INET;

  sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  sockAddr.sin_port = htons(1234);

  connect(sock, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
  cout << "connect sucess" << endl;
  
  //循环接收数据，直到文件传输完毕

  // char buffer[BUF_SIZE] = {0}; //文件缓冲区

  // int nCount;

  // while( (nCount = recv(sock, buffer, BUF_SIZE, 0)) > 0 ){

  // fwrite(buffer, nCount, 1, fp);

  // }

  // cout << "recv sucess" << endl;
    

  char buffer[BUF_SIZE] = {0}; //缓冲区

    int nCount;

    while( (nCount = fread(buffer, 1, BUF_SIZE, fp)) > 0 ){

    send(sock, buffer, nCount, 0);

    }
    cout << "send sucess" << endl;
    
    shutdown(sock, 1); //文件读取完毕，断开输出流，向客户端发送FIN包

    recv(sock, buffer, BUF_SIZE, 0); //阻塞，等待客户端接收完毕

  
  //文件接收完毕后直接关闭套接字，无需调用shutdown()

  fclose(fp);

  close(sock);

  return 0;

}