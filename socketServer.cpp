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

using namespace std;
class CServer
{
private:
  int m_listenfd;
  int m_clientfd;
public:
  CServer();
  ~CServer();
  bool initserver(const int port);
  bool Accept();
  ssize_t Recv(void *buf,ssize_t len);
  ssize_t Send(const void *buf,ssize_t len);
};
CServer::CServer()
{
  m_listenfd=0;
  m_clientfd=0;
}
CServer::~CServer()
{
  if(m_listenfd>0)  close(m_listenfd);
  if(m_clientfd>0)  close(m_clientfd);
}
bool CServer::initserver(const int port)
{
    m_listenfd=socket(AF_INET,SOCK_STREAM,0);//监听
    struct sockaddr_in serveaddr;
    memset(&serveaddr,0,sizeof(serveaddr));
    serveaddr.sin_family=AF_INET;   //ipv4协议族
    serveaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    serveaddr.sin_port=htons(port);
    if(bind(m_listenfd,(const struct sockaddr*)&serveaddr,sizeof(serveaddr))!=0)
    {
      cout << "bind" << endl;
      close(m_listenfd);
      return false;
    }

    if(listen(m_listenfd,5)!=0)
    {
      cout << "listen" << endl;
      close(m_listenfd);
      return false;
    }
    return true;
}
bool CServer::Accept()
{
  int socklen;
  struct sockaddr_in clientaddr;  //客户端的地址信息
  m_clientfd=accept(m_listenfd,(struct sockaddr *)&clientaddr,(socklen_t*)&socklen);
  if(m_clientfd<0)  return false;
  return true;
}
ssize_t CServer::Recv(void *buf,ssize_t len)
{
  memset(buf,0,len);
  return recv(m_clientfd,buf,len,0);
}
ssize_t CServer::Send(const void *buf,ssize_t len)
{
  return send(m_clientfd,buf,len,0);
}
//-------------------------------------
int main()
{
    CServer server;
    if(server.initserver(5000)==false)
    {
      cout <<"server initial" << endl;
      return -1;
    }  
    cout <<"server initial sucess" << endl;
    if(server.Accept()==false)
    {
      cout <<"server Accept" << endl;
      return -1;
    }  
    cout <<"server Accept sucess" << endl;

      char buffer[1024];
      while(true)
      {
        if(server.Recv(buffer,sizeof(buffer))<=0)  
          break;
        printf("收到来信:%s\n",buffer);

        if(strcmp(buffer,"bye")==0)  
          break;

        memset(buffer,0,sizeof(buffer));
        printf("请键入消息并回车键发送:");
        scanf("%s",buffer);
        if(server.Send(buffer,strlen(buffer))<=0)  
          break;
      }
    return 0;
  //}
}