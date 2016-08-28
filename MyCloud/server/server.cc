#include "../public.h"
#include "server.h"
#include <sys/sendfile.h>
#include "processpool.h"
#include "md5.h"
#include "sql.h"

#define HOME_PATH "/home/Swift/Desktop/GitCloud/CloudStore/MyCloud/server/dir/"

static char * g_str = NULL;


//extern int CServer::m_epollfd;

//Public:
CServer::CServer():m_userstat(false)
{

  // while (1)
  // {
  //  int len = sizeof(cliAddr);
  //  cout<<"Waitting for connect\n";

  //  //可以在这里添加进程池或线程池来管理链接，主线程/主进程只创建监听队列并调用accpet()，
  //  //然后便将链接分发给线程池/进程池中进行处理。

     

  //  int cfd = accept(m_sockfd, (struct sockaddr*)&cliAddr, (socklen_t *)&len);
  //  if (cfd == -1)
  //  {
  //    cerr<<"Accept Error!\n";
  //  }

  //  int n = recv(cfd, &info, sizeof(info), 0);
  //  if ( n == -1)
  //  {
  //    cerr<<"Recv Error\n"; 
  //  }

  //  CheckLogin(info, cfd);
  // }
}

CServer::~CServer()
{}

void CServer::init(int epollfd, int sockfd, const sockaddr_in& client_addr, int listen_load)
{
  m_epollfd = epollfd;
  m_sockfd = sockfd;
  m_address = client_addr;
  m_listen_load = listen_load;
}

void CServer::process()
{
  char buffer[4] = {0};
  int n = recv(m_sockfd, buffer, 4, MSG_PEEK);//窥探套接字中的内容，是登录/注册请求还是其它请求
  if (n == -1)
  {
    perror("In Function \" process \" : recv error");
    return ;
  }

  int requestStat = ExplainRequest(buffer);
  
  switch(requestStat) 
  {
    case GET:
      TransmitFile(m_sockfd);
      break;
    case PUT:
      RecvFile(m_sockfd);
      break;
    case SNAP:
      MakeSnapshot(m_sockfd);
      break;
    case BACK:
      BackToSnapshot(m_sockfd);
      break;
    case RM:
      RemoveFile(m_sockfd);
      break;
    case SL:
      SnapList(m_sockfd);
      break;
    case LOGRG:
      CheckLoginOrRegister(m_sockfd);
      break;
    default:
      break;
  }

  //functions[requestStat](m_sockfd);
}

int CServer::ExplainRequest(const char *buffer)
{
  if (strncmp(buffer, "GET", 3) == 0)       return GET;
  else if (strncmp(buffer, "PUT", 3) == 0)  return PUT;
  else if (strncmp(buffer, "SNAP", 4) == 0) return SNAP;
  else if (strncmp(buffer, "BACK", 4) == 0) return BACK;
  else if (strncmp(buffer, "RM", 2) == 0)   return RM;
  else if (strncmp(buffer, "SL", 2) == 0)   return SL;
  else                                      return LOGRG;
}

void CServer::TransmitFile(int cfd)
{
  char requestBuf[BUFSIZE] = {0};
  int nbytes = recv(cfd, requestBuf, BUFSIZE-1, 0);
  if (nbytes == -1)
  {
    perror("In Function \" TransmitFile \" : recv request error");
    return ;
  }
  close(cfd);

  char *ptr = requestBuf + 4;

  char filePath[BUFSIZE] = {0};
  strcpy(filePath, HOME_PATH);
  strcat(filePath, ptr);

  int fd = open(filePath, O_RDONLY);
  if (fd < 0)
  {
    perror("In Function \" TransmitFile\" : open file failed");
    return ;
  }

  struct sockaddr_in client_address;
  socklen_t client_addrlength = sizeof( client_address );
  int connfd = accept( m_listen_load, ( struct sockaddr* )&client_address, &client_addrlength );
  if ( connfd < 0 )
  {
      perror( "Accept Error" );
      return ;
  }

  struct stat statBuf;

  fstat(fd, &statBuf);

  sendfile(connfd, fd, NULL, statBuf.st_size);

  close(connfd);
  close(fd);
}

void CServer::RecvFile(int cfd)
{
  char requestBuf[BUFSIZE] = {0};
  char userName[BUFSIZE] = {0};
  int nbytes = recv(cfd, requestBuf, BUFSIZE-1, 0);
  if (nbytes == -1)
  {
    perror("In Function \" TransmitFile \" : recv request error");
    return ;
  }

  char *ptr = requestBuf + 4; //ptr现在指向用户名
  char *tail = ptr;

  if (requestBuf[strlen(requestBuf)-1] == '/')
  {
    RecvDirection(ptr);
    return ;
  }

  while (*tail != '/') ++tail;

  strncpy(userName, ptr, tail-ptr);

  char filePath[BUFSIZE] = {0};

  strcpy(filePath, HOME_PATH);

  while (*tail != '\0') ++tail;
  strncat(filePath, ptr, tail-ptr+1);
 
  unsigned int md5_src_code[4] = {0};
  unsigned int md5_des_code[4] = {0};
  memcpy(md5_src_code, tail+1, sizeof(md5_src_code));
  
  while (true)
  {
    int fd = open(filePath, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd == -1)
    {
      perror("In Function \"RecvFile\": open file failed");
      return ;
    }

    //recive file
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof( client_address );
    int connfd = accept( m_listen_load, ( struct sockaddr* )&client_address, &client_addrlength );
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
        return ;
    }

    char recvBuff[BUFSIZE] = {0};
    while ( (nbytes = recv(connfd, recvBuff, 64, 0)) > 0)
    {
      int writeBytes = write(fd, recvBuff, nbytes);
      if (writeBytes != nbytes)
      {
        perror("write direction information error");
        return;
      }
      if (nbytes < 64)
      {
        break;
      }
    }

    MD5::getMD5(filePath, (unsigned long *)md5_des_code);

    if (memcmp(md5_src_code, md5_des_code, sizeof(md5_src_code)) == 0)
    {
      send(cfd, "OK", 2, 0);
      close(fd);
      break;
    }
    else
    {
      send(cfd, "NO", 2, 0);
    }

    close(fd);
    close(connfd);
  }

  MakeDirectionText(userName, userName);
  
  close(cfd);
}

void CServer::RecvDirection(char *dir_path)
{
  char current_work_dir[BUFSIZE] = {0};
  static bool tag = true;
  char new_work_dir[BUFSIZE] = {0};

  if (tag)
  {
    strcpy(new_work_dir, HOME_PATH);
    strcat(new_work_dir, dir_path);
    strcat(new_work_dir, "/");
  }
  else
  {
    strcpy(new_work_dir, dir_path);
  }

  umask(0);
  if (mkdir(new_work_dir, 0755) == -1)
  {
    printf("%s\n", new_work_dir);
    perror("In Function \"RecvDirection\" : make direction error");
    return ;
  }
  getcwd(current_work_dir, BUFSIZE-1);

  chdir(new_work_dir);


  chdir(current_work_dir);
}

//RemoveFile的思想是，删除文件时并不真正删除文件，而是删除当前快照中对应的记录。这样就相当于删除了文件
void CServer::RemoveFile(int cfd)
{
  char requestBuf[BUFSIZE] = {0};

  int nbytes = recv(cfd, requestBuf, BUFSIZE-1, 0);
  if (nbytes == -1)
  {
    perror("In Function \" TransmitFile \" : recv request error\n");
    return ;
  }
  close(cfd);

  char *ptr = requestBuf + 3;   //使ptr指向用户名
  char *space_pos = ptr;

  while (*space_pos != '\0' && *space_pos != ' ') ++space_pos;
  --space_pos;

  char filePath[BUFSIZE] = {0};
  char dirinfoPath[BUFSIZE] = HOME_PATH;
  char tmpdirInfo[BUFSIZE] = HOME_PATH;
  char userName[BUFSIZE] = {0};
  strncpy(userName, ptr, space_pos-ptr+1);
  strcpy(filePath, userName);
  strcat(filePath, "/");

  ptr = space_pos + 2;//使ptr指向待删除文件名

  if ( IsNeedRealyRemove(userName, ptr) )
  {
    RealyRemove(userName, ptr);//judge this file need to realy remove or not
  }

  size_t compare_size = strlen(ptr);
  strcat(filePath, ptr);

  strcat(dirinfoPath, userName);
  strcat(dirinfoPath, "/.dirinfo");

  strcat(tmpdirInfo, userName);
  strcat(tmpdirInfo, "/.tmpdirInfo");

  //dirinfo路径准备完毕，newdirInfo也准备完毕
  
  ifstream userfile;
  userfile.open(dirinfoPath, ios::in);

  umask(0);
  int fd = open(tmpdirInfo, O_WRONLY|O_CREAT|O_TRUNC, 0666);

  if (fd == -1)
  {
    perror("In Function \"RemoveFile\" : open tmpdirInfo error");
    return ;
  }

  char buffer[BUFSIZE] = {0};//每次读取userinfo文件的一行,并保存在buffer中

  while ( !(userfile.eof()) )
  {
    userfile.getline(buffer, BUFSIZE-1);
    strcat(buffer, "\n");
    int buf_len = strlen(buffer);

    if (strncmp(buffer, ptr, compare_size-1) == 0)
    { 
      //printf("%s", buffer);
      continue;
    }

    int nbytes = write(fd, buffer, buf_len);
    if (nbytes != buf_len || nbytes == -1)
    {
      perror("In Function \"RemoveFile\" : write to tmpdirInfo failed");
      return ;
    }
  }

  int res = remove(dirinfoPath);
  if (res == -1)
  {
    perror("In Function \"RemoveFile\" : unlink error");
    return ;
  }
  res = rename(tmpdirInfo, dirinfoPath);
  if (res == -1)
  {
    perror("In Function \"RemoveFile\" : rename error");
    return ;
  }

  close(fd);
  userfile.close();
}

bool CServer::IsNeedRealyRemove(const char *username, const char *file_path)
{
  int compare_size = strlen(file_path);

  char filePath[BUFSIZE] = HOME_PATH;
  strcat(filePath, username);
  strcat(filePath, "/");
  
  struct stat statbuf;
  DIR *dp;
  struct dirent *entry;

  if ( (dp = opendir(filePath)) == NULL )
  {
    perror("Opendir Error");
    return false;
  }

  while ((entry = readdir(dp)) != NULL)
  {
    if ( strncmp(entry->d_name, ".SNAP-", 6) != 0 )
    {
      continue;
    }
    else
    {
      //printf("%s\n", entry->d_name);
      char snapfile[BUFSIZE] = {0};
      strcpy(snapfile, filePath);
      strcat(snapfile, entry->d_name);
      
      ifstream snapshot;
      char tmpBuf[BUFSIZE] = {0};
      
      snapshot.open(snapfile, ios::in);
      if (!snapshot)
      {
        perror("Open snapshot error");
      }

      while ( !snapshot.eof() )
      {
        snapshot.getline(tmpBuf, BUFSIZE-1);
        if (tmpBuf[0] == '\0') break;
        

       // printf("%s\t%s\n", tmpBuf, file_path);
        if ( strncmp(tmpBuf, file_path, compare_size) == 0 )//can't remove file
        {
          //printf("find %s\n", file_path);
          return false;
        }

        memset(tmpBuf, 0, BUFSIZE);
      }
      snapshot.close();
    }
  }
  return true;
}

void CServer::RealyRemove(const char *username, const char *file_path)
{
  static bool tag = true;
  char filePath[BUFSIZE] = {0};
  if (tag)
  {
    strcpy(filePath, HOME_PATH);
    strcat(filePath, file_path);
    tag = false;
  }
  else
  {
    strcpy(filePath, file_path);
  }
 
  struct stat statbuf;
  lstat(filePath, &statbuf);

  if ( S_ISDIR(statbuf.st_mode) )//removing direction
  {
    DIR *dp = NULL;
    struct dirent *entry = NULL;
    if ( (dp=opendir(filePath)) == NULL )
    {
      perror("In Function \"RealyRemove\" : open direction error");
      return ;
    }

    char tmpBuf[BUFSIZE] = {0};
    strcpy(tmpBuf, filePath);
    while ( (entry=readdir(dp)) != NULL )
    {
      if ( (strcmp(entry->d_name, ".")==0) || (strcmp(entry->d_name, "..")==0) )
        continue;
      
      strcat(filePath, "/");
      strcat(filePath, entry->d_name);

      lstat(filePath, &statbuf);

      if ( S_ISDIR(statbuf.st_mode) )
      {
        RealyRemove(username, filePath);
      }
      else
      {
        if (remove(filePath) == -1)
        {
          printf("%s\n", filePath);
          perror("In Function \"RealyRemove\" : remove regular file error");
          return ;
        }
      }

      memset(filePath, 0, BUFSIZE);
      strcpy(filePath, tmpBuf);
    }
    if (remove(tmpBuf) == -1)
    {
      printf("%s\n", filePath);
      perror("In Function \"RealyRemove\" : remove direction error");
      return ;
    }
  }
  else
  {
    if (remove(filePath) == -1)
    {
      printf("%s\n", filePath);
      perror("In Function \"RealyRemove\" : remove regular file error");
      return ;
    }
  }
}
/*
void CServer::RealyRemove(const char *username, const char *file_path)
{
  static bool tag = true;

  char filePath[BUFSIZE] = {0};
  if (tag)
  {
    strcpy(filePath, HOME_PATH);
    tag = false;
  }
  else
  {
    strcpy(filePath, file_path);
  }
  
  strcat(filePath, file_path);
  
  struct stat statbuf;

  lstat(filePath, &statbuf);


  
  if ( S_ISDIR(statbuf.st_mode) )//removing direction
  {
    DIR *dp;
    struct dirent *entry;

    
    if ( (dp = opendir(filePath)) == NULL )
    {
      perror("Opendir Error");
      return ;
    }

    char tmpBuf[BUFSIZE] = {0};
    strcpy(tmpBuf, filePath);

    while ((entry = readdir(dp)) != NULL)
    {
      if ( (strcmp(entry->d_name, ".")==0) || (strcmp(entry->d_name, "..")==0) )
      {
        continue ;
      }
      strcat(filePath, "/");
      strcat(filePath, entry->d_name);
      lstat(filePath, &statbuf);

      if (S_ISDIR(statbuf.st_mode))//removing sub_direction
      {
        RealyRemove(username, filePath);
        if (remove(filePath) == -1)
        {
          printf("%s\n", filePath);
          perror("In Function \"RealyRemove\" : remove sub_direction error");
          return ;
        }
      }
      else//removing sub_regular file
      {
        if (remove(filePath) == -1)
        {
          printf("%s\n", filePath);
          perror("In Function \"RealyRemove\" : remove sub_regular file error");
          return ;
        }
      }

      memset(filePath, 0, BUFSIZE);
      strcpy(filePath, tmpBuf);
    }

    if (remove(tmpBuf) == -1)
    {
      printf("%s\n", tmpBuf);
      perror("In Function \"RealyRemove\" : remove direction error");
      return ;
    }
  }
  else//removing regular file
  {
    if (remove(filePath) == -1)
    {
      printf("%s\n", filePath);
      perror("In Function \"RealyRemove\" : remove regular file error");
      return ;
    }
  }
}
*/

void CServer::MakeSnapshot(int cfd)
{
  char requestBuf[BUFSIZE] = {0};

  int nbytes = recv(cfd, requestBuf, BUFSIZE-1, 0);
  if (nbytes == -1)
  {
    perror("In Function \" TransmitFile \" : recv request error\n");
    return ;
  }
  close(cfd);

  char *ptr = requestBuf + 5;   //使ptr指向用户名
  char *space_pos = ptr;

  while (*space_pos != '\0' && *space_pos != ' ') ++space_pos;
  --space_pos;

  char srcPath[BUFSIZE] = HOME_PATH;
  char desPath[BUFSIZE] = HOME_PATH;
  char userName[BUFSIZE] = {0};
  strncpy(userName, ptr, space_pos-ptr+1);
  strcat(srcPath, userName);
  strcat(srcPath, "/.dirinfo");

  ptr = space_pos + 2;     //使ptr指向快照名
  strcat(desPath, userName);
  strcat(desPath, "/.SNAP-");
  strcat(desPath, ptr);

  char readBuff[BUFSIZE] = {0};

  //printf("%s\n%s\n", srcPath, desPath);///////////////////////////////

  int srcfd = open(srcPath, O_RDONLY);

  int desfd = open(desPath, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (srcfd == -1 || desfd==-1)
  {
    perror("In Function \"MakeSnapshot\" : open dirinfo failed\n");
    return ;
  }

  while ((nbytes = read(srcfd, readBuff, 64)) > 0)
  {
    write(desfd, readBuff, nbytes);
    memset(readBuff, 0, BUFSIZE);
  }

  close(srcfd);
  close(desfd);
}

void CServer::BackToSnapshot(int cfd)
{
  char requestBuf[BUFSIZE] = {0};
  char srcPath[BUFSIZE] = HOME_PATH;  
  char desPath[BUFSIZE] = HOME_PATH;

  int nbytes = recv(cfd, requestBuf, BUFSIZE-1, 0);
  if (nbytes == -1)
  {
    perror("In Function \" TransmitFile \" : recv request error\n");
    return ;
  }
  
  char *ptr = requestBuf + 5;   //使ptr指向用户名
  char *space_pos = ptr;

  while (*space_pos != '\0' && *space_pos != ' ') ++space_pos;
  
  --space_pos;

  //拼接srcPath路径
  char userName[BUFSIZE] = {0};
  strncpy(userName, ptr, space_pos-ptr+1);
  strcat(srcPath, userName);
  strcat(srcPath, "/.dirinfo");

  //拼接despath路径
  strcat(desPath, userName);
  strcat(desPath, "/.current_dirinfo");

  //拼接newDirinfo路径
  ptr = space_pos + 2;     //使ptr指向快照名
  char newDirinfo[BUFSIZE] = HOME_PATH;
  strcat(newDirinfo, userName);
  strcat(newDirinfo, "/.SNAP-");
  strcat(newDirinfo, ptr);

  int res = rename(srcPath, desPath);//备份当前文件状态
  if (res == -1)
  {
    printf("rename error: %s\nto\n%s\n ",srcPath, desPath);
  }

  char readBuff[BUFSIZE] = {0};
  int srcfd = open(newDirinfo, O_RDONLY);

  int desfd = open(srcPath, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (srcfd == -1 || desfd==-1)
  {
    perror("In Function \"MakeSnapshot\" : open dirinfo failed\n");
    return ;
  }

  while ((nbytes = read(srcfd, readBuff, 64)) > 0)
  {
    write(desfd, readBuff, nbytes);
    memset(readBuff, 0, BUFSIZE);
  }
  close(srcfd);
  close(desfd);

  // res = rename(newDirinfo, srcPath);//使快照状态恢复至用户要求的状态
  // if (res == -1)
  // {
  //  printf("rename error: %s\nto\n%s\n ",newDirinfo, srcPath);
  // }
  
  SendDirectionText(cfd, userName);

  close(cfd);
}

void CServer::SnapList(int cfd)
{
  char requestBuf[BUFSIZE] = {0};
  char echoBuf[BUFSIZE] = {0};

  int nbytes = recv(cfd, requestBuf, BUFSIZE-1, 0);
  if (nbytes == -1)
  {
    perror("In Function \" TransmitFile \" : recv request error\n");
    return ;
  }

  char *ptr = requestBuf + 3;//point to username

  char userPath[BUFSIZE] = HOME_PATH;

  strcat(userPath, ptr);
  
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;

  if ( (dp = opendir(userPath)) == NULL )
  {
    perror("Opendir Error");
    printf("%s\n", userPath);
    return ;
  }

  while ( (entry = readdir(dp)) != NULL )
  {
    if ( getcwd(userPath, BUFSIZE) == NULL )
    {
      perror("getcwd error!\n");
      return ;
    }
    
    lstat(entry->d_name, &statbuf);
  
    if (strncmp(entry->d_name, ".SNAP-", 6) == 0)//this file is a snapshot
    {
      char *temp = entry->d_name + 1;
      strcat(echoBuf, temp);
      strcat(echoBuf, "  ");
    }
  }

  nbytes = send(cfd, echoBuf, strlen(echoBuf), 0);
  if ( nbytes == -1 )
  {
    perror("In Function \"SnapList\" : send echo buffer error");
    return ;
  }

  close(cfd);
}
void CServer::CheckLoginOrRegister(int cfd)
{
  Userinfo info;
  int n = recv(cfd, &info, sizeof(info), 0);
  assert (n != -1);
  if (info.model == LOG)
  {
    UserLogin(cfd, info);//处理用户登录
  }
  else if (info.model == REG)
  {
    UserRegister(cfd, info);//处理用户注册
  }
  else
  {
    printf("In Function \"CheckLoginOrRegister\" : model error\n");
  }
}
void CServer::UserLogin(int cfd, Userinfo &userinfo)
{
  LOGIN_STA statu = USRERR;

  ConnMySQL db;

  char sql_query[BUFSIZE] = "select * from userinfo where username=";
  char temp_buf[BUFSIZE] = "'";
  strcat(temp_buf, userinfo.userName);
  strcat(temp_buf, "'");
  strcat(sql_query, temp_buf);

  char query_name[32] = {0};
  char query_password[32] = {0};

  int ret_stat = db.Query(sql_query, query_name, query_password);

  if (ret_stat == SUCCESS)
  {
    statu = PASSERR;
    if ( strcmp(query_password, userinfo.password) == 0 )
    {
      statu = USRSUC;
      m_userstat = true;
    }
  }

  int nbytes = send(cfd, &statu, sizeof(statu), 0);
  assert(nbytes != -1);
  if (m_userstat)
  {
    SendDirectionText(cfd, query_name);
  }

  close(cfd);
}

/*
void CServer::UserLogin(int cfd, Userinfo &userinfo)
{
  LOGIN_STA statu = USRERR;
  char userName[BUFSIZE] = {0};

  ifstream userfile;
  userfile.open("../userinfo", ios::in);

  char buffer[BUFSIZE] = {0};//每次读取userinfo文件的一行,并保存在buffer中
  char userNameBuf[BUFSIZE] = {0};
  char userPassBuf[BUFSIZE] = {0};

  while ( !userfile.eof() )
  {
    userfile.getline(buffer, BUFSIZE-1);
    if (buffer[0] == '\0') break;

    char * pstr = buffer;
        bool nameStatu = false;
        bool passStatu = false;
      
        pstr = strtok(buffer, ":");
        strcpy(userNameBuf, pstr);

        pstr = strtok(NULL, ":");
        strcpy(userPassBuf, pstr);

        if (strcmp(userNameBuf, userinfo.userName) == 0)
        {
          statu = PASSERR;
          if (strcmp(userPassBuf, userinfo.password) == 0)
          {
            statu = USRSUC;
            strcpy(userName, userNameBuf);
            m_userstat = true;
            break;
          }
          break;//密码错误也应该break
        }

        memset(buffer, 0, 128);
        memset(userNameBuf, 0, BUFSIZE);
        memset(userPassBuf, 0, BUFSIZE);
  }

  int nbytes = send(cfd, &statu, sizeof(statu), 0);
  assert(nbytes != -1);
  if (m_userstat)
  {
    SendDirectionText(cfd, userName);
  }

  userfile.close();
  close(cfd);
}
*/
void CServer::SendDirectionText(int cfd, char *userName)
{
  char filePath[BUFSIZE] = HOME_PATH;//将每个文件的路径保存在这个文本中  
  strcat(filePath, userName);
  strcat(filePath, "/.dirinfo");
  int fd = open(filePath, O_RDONLY);
  /*
  if (fd < 0)
  {
    //MakeDirectionText(userName, userName); //user can't delete their .dirinfo,so we don't need to create 
    //                                       //.dirinfo every times
    //if( open(filePath, O_RDONLY) == -1)
    //{
      perror("In Function \"SendDirectionText\" : open dirinfo failed");
      return ;
    //}
  }
  */

  struct stat statBuf;

  fstat(fd, &statBuf);

  sendfile(cfd, fd, NULL, statBuf.st_size);

  close(fd);
}

void CServer::UserRegister(int cfd, Userinfo &userinfo)
{
  ConnMySQL db;

  char sql_query[BUFSIZE] = "select * from userinfo where username=";
  char temp_buf[BUFSIZE] = "'";
  strcat(temp_buf, userinfo.userName);
  strcat(temp_buf, "'");
  strcat(sql_query, temp_buf);
  
  char userDirPath[BUFSIZE] = HOME_PATH;
  strcat(userDirPath, userinfo.userName);

  if ( db.Query(sql_query, NULL, NULL) == NOUSER)
  {
    umask(0);
    mkdir(userDirPath, 0755); 

    char sql_insert[BUFSIZE] = "insert into userinfo values";
    memset(temp_buf, 0, BUFSIZE);

    strcpy(temp_buf, "('");
    strcat(temp_buf, userinfo.userName);
    strcat(temp_buf, "','");
    strcat(temp_buf, userinfo.password);
    strcat(temp_buf, "')");

    strcat(sql_insert, temp_buf);
    printf("%s\n", sql_insert);
    db.Insert(sql_insert);

    int nbytes = send(cfd, "succe", 5, 0);
    
    if (nbytes <= 0)
    {
      perror("In Function \"UserRegister\" : send error");
      return ;
    }
  }
  else
  {
    int nbytes = send(cfd, "exist", 5, 0);

    if (nbytes <= 0)
    {
      perror("In Function \"UserRegister\" : send error");
      return ;
    }
  }

  close(cfd);
}

/*
void CServer::UserRegister(int cfd, Userinfo &userinfo)
{
  ofstream userfile;
  //ios_base::out会清空当前文件所有数据,然后再向文件写数据。ios_base::app会以添加的方式打开文件
  userfile.open("../userinfo", ios_base::app);

  if (!userfile)
  {
    perror("Open userinfo failed");
    exit(1);
  }

  char buffer[BUFSIZE] = {0};
  char userDirPath[BUFSIZE] = HOME_PATH;
  strcat(userDirPath, userinfo.userName);

  strcpy(buffer, userinfo.userName);
  strcat(buffer, ":");
  strcat(buffer, userinfo.password);
  strcat(buffer, "\n");

  mkdir(userDirPath, 0755); 

  userfile<<buffer;
  userfile.close();
  close(cfd);
}
*/

void CServer::UserInterface(int cfd)
{
  char userCommand[BUFSIZE] = {0};
  char echoBuffer[BUFSIZE*10] = {0};
  int  nbytes = 0;
  bool running = true;

  while (running)
  {
    nbytes = recv(cfd, userCommand, BUFSIZE-1, 0);
    if (nbytes == -1)
    {
      if (errno != EAGAIN)//发生了错误,并且错误不是EAGAIN,说明recv真正失败,需要结束本次链接。
      {
        removefd(m_epollfd, m_sockfd);
      }
      break;
    }
    else if (nbytes == 0)//说明recv没有读到任何数据
    {
      removefd(m_epollfd, m_sockfd);
    }
    else
    {
      if (strncmp(userCommand, "exit", 4) == 0)
      {
        running = false;
        m_userstat = false;
      }

      cout<<userCommand<<endl;
      if (running)
      {
        //temp operation !!!!
        nbytes = send(cfd, "OK", 2, 0);
        if (nbytes == -1)
        {
          perror("UserInterface send failed");
          running = false;
        }
      }
    }

    memset(userCommand, 0, BUFSIZE);
    memset(echoBuffer, 0, BUFSIZE*10);
  }
}


void CServer::MakeDirectionText(char *userName, char *subpath)
{
  assert(userName != NULL && subpath != NULL);

  static bool tag = true;
  static bool flag = true;

  char filePath[BUFSIZE] = HOME_PATH;//将每个文件的路径保存在这个文本中  
  strcat(filePath, userName);
  strcat(filePath, "/.dirinfo");
  int fd = -1;

  if (flag)//递归第一次重新清除dirinfo中的数据,因
  {
    fd = open(filePath, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    flag = false;
  }
  else//之后的递归就不用清除数据
  {
    fd = open(filePath, O_WRONLY|O_APPEND);
  }
  if (fd == -1)
  {
    perror("open user home file error");
    return ;
  }

  char pathBuf[BUFSIZE] = {0};          //用来保存每个文件的路径    

  char userPath[BUFSIZE] = {0};         //server端当前工作目录

  if ( tag )
  {
    strcpy(userPath, HOME_PATH);
    strcat(userPath, userName);
    tag = false;
  }
  else
  {
    if ( getcwd(userPath, BUFSIZE) == NULL)
    {
      perror("getcwd error");
      return ;
    }
    strcat(userPath, "/");
    strcat(userPath, subpath);
  }

  chdir(userPath);

  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;

  if ( (dp = opendir(userPath)) == NULL )
  {
    perror("Opendir Error");
    printf("%s\n", userPath);
    return ;
  }

  while ( (entry = readdir(dp)) != NULL )
  {
    if ( getcwd(userPath, BUFSIZE) == NULL )
    {
      perror("getcwd error!\n");
      return ;
    }
    lstat(entry->d_name, &statbuf);

    if ( S_ISDIR(statbuf.st_mode) )
    {
      if ( strcmp(".", entry->d_name)==0 || strcmp("..", entry->d_name)==0 )
      {
        continue;
      }

      MakeDirectionText(userName, entry->d_name);//如果是目录，就递归调用MakeDirectionText,遍历子目录的文件
      lseek(fd, 0, SEEK_END);//递归回来之后，在这个递归栈上他的偏移量需要设置到当前文件尾部，否则会覆盖之前写进去的内容
    }

    strcat(userPath, "/");
    strcat(userPath, entry->d_name);
    if ( S_ISDIR(statbuf.st_mode) )//目录文件与普通文件在dirinfo中的区别就是目录文件以'/'结尾
    {
      strcat(userPath, "/");        
    }
    char *pstr = mystrtok(userPath, "/");

    while ( (pstr = mystrtok(NULL, "/")) != NULL )
    {
      if ( strncmp(pstr, userName, strlen(userName)) == 0 )
      {
        strcpy(pathBuf, pstr);
        strcat(pathBuf, "\n");
        break;
      }
    }

    if (pstr != NULL)
    {
      int nbytes = write(fd, pathBuf, strlen(pathBuf));

      memset(userPath, 0, BUFSIZE);
      memset(pathBuf, 0, BUFSIZE);

      if (nbytes < 0)
      {
        perror("write to DirectionText error\n");
        return ;
      }
    }
  }

  chdir("..");
  closedir(dp);
  close(fd);
}

char* CServer::mystrtok(char *str, const char *delim)
{
  if (delim == NULL) return NULL;

  if (str == NULL) str = g_str;

  size_t len = strlen(delim);

  char *rt_ptr = NULL;

  while (str++ != NULL)
  {
    if (strncmp(str, delim, len) == 0)
    {
      g_str = str+1;
      rt_ptr = str+1;
      break;
    }
  }

  return rt_ptr;
}

int main()
{
  //创建CTP(控制端口)监听套接字
  int listenfdCTP = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfdCTP == -1)
  {
    perror("Create socket failed!");
  }

  struct sockaddr_in serAddrCTP;
  //struct sockaddr_in cliAddr;
  memset(&serAddrCTP, 0, sizeof(serAddrCTP));

  serAddrCTP.sin_family = AF_INET;
  serAddrCTP.sin_port = htons(CTP);
  serAddrCTP.sin_addr.s_addr = inet_addr("192.168.0.128");

  int res = bind(listenfdCTP, (struct sockaddr*)&serAddrCTP, sizeof(serAddrCTP));

  if (res == -1)
  {
    perror("Bind Error!");
    exit(1);
  }

  listen(listenfdCTP, 5);


  //创建(DLP)上传下载套接字
  int listenfdDLP = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfdDLP == -1)
  {
    perror("Create socket failed!");
  }

  struct sockaddr_in serAddrDLP;
  //struct sockaddr_in cliAddr;
  memset(&serAddrDLP, 0, sizeof(serAddrDLP));

  serAddrDLP.sin_family = AF_INET;
  serAddrDLP.sin_port = htons(DLP);
  serAddrDLP.sin_addr.s_addr = inet_addr("192.168.0.128");

  res = bind(listenfdDLP, (struct sockaddr*)&serAddrDLP, sizeof(serAddrDLP));

  if (res == -1)
  {
    perror("Bind Error!");
    exit(1);
  }

  listen(listenfdDLP, 5);


  //////////////////////////////////////
  processpool< CServer > *pool = processpool< CServer >::create(listenfdCTP, listenfdDLP);

  if ( pool != NULL )
  {
    pool->run();
    delete pool;
  } 
  return 0;
}
