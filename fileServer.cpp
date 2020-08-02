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

    //先检查文件是否存在

    //char *filename = "123.txt"; //文件名
    


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
    if(clntSock == -1)
    {
      cout << "accept failed" << endl;
      return 0;
    }

    string str;
    str = "HTTP/1.1 200 OK\r\n";
    str = str + "Access-Control-Allow-Origin: null\r\n";
    str = str + "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n";
    str = str + "Access-Control-Allow-Headers: X-PINGOTHER, Content-Type\r\n";
    str = str + "Content-Encoding: gzip, deflate\r\n";
    str = str + "Connection: Keep-Alive\r\n";
    cout << str << endl;

    int nCount1 = send(clntSock, str.c_str(),str.size(), 0);
    cout << "accept sucess" << endl;

   if(nCount1 <=0)  
   {
     cout << "send: failed" << endl;
     return 0;
   }
    cout << "send: sucess" << endl;

    close(clntSock);

  while(1){

    int clntSock = accept(servSock, (struct sockaddr *)&clientaddr, (socklen_t*)&socklen);
    if(clntSock == -1)
    {
      continue;
      cout << "clntSock == -1" << endl;
    }

    
    //循环接收数据，直到文件传输完毕
  char buffer[BUF_SIZE] = {0}; //文件缓冲区
  int nCount = recv(clntSock, buffer, BUF_SIZE, 0);
   cout<< nCount << endl;
    string szTemp("");  
    szTemp = "";  
    szTemp += buffer;  
    cout << szTemp;  

  FILE *fp = fopen("123.txt", "wb"); //以二进制方式打开文件

    if(fp == NULL){
    printf("Cannot open file, press any key to exit!\n");
    exit(0);
    }
  fwrite(buffer, nCount, 1, fp);
  cout << "fwrite: sucess" << endl;
  close(clntSock);
  fclose(fp);
  }


  cout << "recv sucess" << endl;

    close(servSock);

    cout << "send end" << endl;

    return 0;

}

