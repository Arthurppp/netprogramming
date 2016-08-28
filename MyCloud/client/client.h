#ifndef __CLIENT_H__
#define __CLIENT_H__
#include "public.h"
#include "file.h"

class CClient
{
public:
  CClient();
  ~CClient()
  {
    file_tree.DestroyFileTree();
    shutdown(m_sockfd, SHUT_RDWR);
  } 

public:
  void ls();
  bool cd(const char *pathname);
  bool download(const char *parameter);
  bool upload(const char *parameter);
  void lslocal(const char *pathname);
  bool snapshot(const char *snapname);
  bool backto(const char *snapname);
  bool remove(const char *file_name);
  bool snaplist();
private:
  int m_sockfd;
  int m_loadfd;
  char m_name[BUFSIZE];
  pid_t m_daemon;
  FileStruct file_tree;
  std::list<__file_node>* current_pos;

private:
  //This is Menu interface of client
  void CreateSocket(int port);
  void Menu();
  void ClientLogin();
  void ClientRegistration();
  void UserOperation();
  void CreateDirectionText();
  void MakeDirectionTree();
  bool GetParameter(const char *command, char *arr);
  void ExecuteCommand(const char *command);
  void InitClient();
  bool UploadDirection(const char *file_name);
  //bool GetLoadParameters(const char *command, char *parameter[BUFSIZE]);//之后需要一次上传或下载多个文件的时候再实现该函数
};
#endif
