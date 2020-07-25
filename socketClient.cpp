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

class CClient
{
private:
  int m_clientfd;
public:
  CClient();
  ~CClient();
  bool connect_to_server(const char *serveip,const int port);
  ssize_t Recv(void *buf, size_t len);
  ssize_t Send(const void *buf, size_t len);
};
CClient::CClient()
{
  m_clientfd=0;
}
CClient::~CClient()
{
  if(m_clientfd>0)  close(m_clientfd);
}
bool CClient::connect_to_server(const char *serveip,const int port)
{
  m_clientfd=socket(AF_INET,SOCK_STREAM,0);
  struct sockaddr_in serveaddr;
  memset(&serveaddr,0,sizeof(serveaddr));
  serveaddr.sin_family=AF_INET;   //ipv4协议族
  serveaddr.sin_port=htons(port);
  if(connect(m_clientfd,(struct sockaddr *)&serveaddr,sizeof(serveaddr))!=0)
  {
    perror("connect");
    close(m_clientfd);
  return false;
  }
return true;
}
ssize_t CClient::Recv(void *buf,size_t len)
{
  memset(buf,0,len);
  return recv(m_clientfd,buf,len,0);
}
ssize_t CClient::Send(const void *buf,size_t len)
{
  return send(m_clientfd,buf,len,0);
}
//------------------------------------------------------
int main( )
{
  CClient client;
  if(client.connect_to_server("127.0.0.1",5000)==false)  return -1;
  char buffer[1024];
  while(true)
  {
    printf("请键入消息回车键发送:");
    scanf("%s",buffer);
    if(client.Send(buffer,strlen(buffer))<=0)  break;
    memset(buffer,0,sizeof(buffer));
    if(client.Recv(buffer,sizeof(buffer))<=0)  break;
    printf("收到来信:%s\n",buffer);
  }
return 0;
}