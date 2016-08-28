#ifndef SERVER_H
#define SERVER_H

#include "../public.h"

#define   BAD    0
#define   GET    1
#define   PUT    2
#define   SNAP   3
#define   BACK   4
#define   REMV   5
#define   RM     6
#define   SL     7
#define   LOGRG  8

typedef void (*Function)(int sockfd);

/*
typedef struct FuncTable
{

}FuncTable;
*/

class CServer
{
public:
  CServer();
  ~CServer();
  void init(int epollfd, int sockfd, const sockaddr_in& client_addr, int listen_load);
  void process();
private:
  void CheckLogin(int cfd);
  void UserInterface(int cfd);
  int ExplainRequest(const char *buffer);
  static void MakeDirectionText(char *userName, char *path);
  static char* mystrtok(char *str, const char *delim);
  void RecvDirection(char *dir_path);
  void UserLogin(int cfd, Userinfo &userinfo);
  void UserRegister(int cfd, Userinfo &userinfo);
  void SendDirectionText(int cfd, char *userName);
  void RealyRemove(const char *username, const char *file_path);
  bool IsNeedRealyRemove(const char *username, const char *file_path);
private:
  void TransmitFile(int cfd);
  void RecvFile(int cfd);
  void MakeSnapshot(int cfd);
  void BackToSnapshot(int cfd);
  void RemoveFile(int cfd);
  void SnapList(int cfd);
  void CheckLoginOrRegister(int cfd);
  
private:
  //int m_listenfd;//监听套接字

  int m_sockfd;//已连接套接字
  int m_listen_load;//上传下载监听套接字
  bool m_userstat;//本次服务对应的客户端状态

  //string m_serAddr;

  static Function *functions;
  sockaddr_in m_address;

  static int m_epollfd;
};

/*
Function *CServer::functions = new Function [10];

CServer::functions[0] = CServer::TransmitFile;
CServer::functions[1] = CServer::RecvFile;
CServer::functions[2] = CServer::MakeSnapshot;
CServer::functions[3] = CServer::BackToSnapshot;
CServer::functions[4] = CServer::RemoveFile;
CServer::functions[5] = CServer::SnapList;
CServer::functions[6] = CServer::CheckLoginOrRegister;
*/

int CServer::m_epollfd = -1;

#endif
