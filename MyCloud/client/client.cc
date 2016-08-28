#include "../public.h"
#include "client.h"
#include "md5.h"

#include <sys/sendfile.h>

//\033[34;48m string \033[0m

//default constructor
CClient::CClient():m_sockfd(-1),m_loadfd(-1)
{
  //创建套接字向服务器发起连接，打印菜单，判断客户的选项，向服务器发起对应的请求即可。连接建立并且登录
  //成功之后便进入命令行模式(使用echo模式，服务器向客户端发送一段字符串，在此模式下，客户端可以输入命令)
  //输入的命令会通过套接字发送给服务器，服务器接收到客户端发来的数据之后便解析该数据，执行对应的命令之后
  //将执行结果通过套接字发送给客户端即可。如果客户端进行下载或者上传时，客户端和服务器会创建另外一个连接
  //专门用来下载和上传(下载和上传可以使用两个不同的端口实现，这样的话便会有3个端口号，分别为：UI端口，
  //下载端口，上传端口。UI端口一直连接着，直到用户退出的客户端的时候连接才会断开。而下载和上传连接只有在
  //客户端下载和上传的过程中保持连接，当客户端停止下载和上传之后便断开连接)。


  /*
    发起与服务器端的链接
  */

  /*
    选择登录选项 1.登录 2.注册
  */
  CreateSocket(CTP);
  Menu(); 
}

//创建套接字
void CClient::CreateSocket(int port)
{
   //如果套接字没有关闭，就不需要重新创建
   if (port == CTP)
   {
    if (m_sockfd != -1)
    {
      return ;
    }
   }
   else
   {
    if (m_loadfd != -1)
    {
      return ;
    }
   }
  
  if (port == CTP)
  {
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (m_sockfd == -1)
    {
      perror("Create socket failed!");
    }
  }
  else
  {
    m_loadfd = socket(AF_INET, SOCK_STREAM, 0);

    if (m_loadfd == -1)
    {
      perror("Create socket failed!");
    }
  }

  struct sockaddr_in serAddr;
  struct sockaddr_in cliAddr;
  memset(&serAddr, 0, sizeof(serAddr));

  serAddr.sin_family = AF_INET;
  serAddr.sin_port = htons(port);
  serAddr.sin_addr.s_addr = inet_addr("192.168.0.128");

  int res = 0;

  if (port == CTP)
  {
    res = connect(m_sockfd, (struct sockaddr*)&serAddr, sizeof(serAddr));
  }
  else
  {
    res = connect(m_loadfd, (struct sockaddr*)&serAddr, sizeof(serAddr));
  }

  if (res == -1)
  {
    perror("connect failed");
  }
}

//This is Menu interface of client
void CClient::Menu()
{
  int choose = 0;

  cout<<"Welcome to cloud storage system"<<endl;
  cout<<"1.Login"<<endl;
  cout<<"2.Registration"<<endl;
  cout<<"Please choose what you want to do: [ ]\b\b";

  cin>>choose;

  switch(choose)
  {
    case 1: ClientLogin();break;
    case 2: ClientRegistration();break;
    default:
        cout<<"Error input"<<endl;break;
  }
}

//用户登录接口
void CClient::ClientLogin()
{
  Userinfo user;
  memset(&user, 0, sizeof(user));
  user.model = LOG;
  cout<<"Please input username:";
  //fgets(user.userName, BUFSIZE, stdin); 
  //user.userName[strlen(user.userName)-1] = '\0';
  fflush(stdin);
  cin>>user.userName;

  cout<<"Please input password:";
  //fgets(user.password, BUFSIZE, stdin); 
  //user.password[strlen(user.password)-1] = '\0';
  fflush(stdin);
  cin>>user.password;

  strcpy(m_name, user.userName);

  InitClient();

  int res = send(m_sockfd,&user,sizeof(user),0);
  if(res == -1)
  {
    perror("Send error");
  }
  char buff[BUFSIZE] = {0};
  //延时是否需要?
  LOGIN_STA sta = USRERR;
  recv(m_sockfd,&sta,sizeof(sta),0);
  switch(sta)
  {
    case USRERR:
        cout<<"\033[0;41;1m Can't find user \033[0m"<<endl;
        break;
    case PASSERR:
        cout<<"\033[0;41;1m Wrong password \033[0m"<<endl;
        break;
    case USRSUC:
        cout<<"\033[42;37;1m Login succeed \033[0m"<<endl;
        cout<<"\n\n\033[46;30;1m ^_^ Welcome to HerCloud ^_^\033[0m"<<endl;
        UserOperation();
        break;
    default:
        break;
  }

  close(m_sockfd);
}

void CClient::InitClient()
{
  umask(0);
  mkdir("./cloud", 0755);
  char dirPath[BUFSIZE] = {0};
  strcpy(dirPath, "./cloud/");
  strcat(dirPath, m_name);
  mkdir(dirPath, 0755);
}

//用户注册，输入用户名和密码，在服务器端注册用户。
void CClient::ClientRegistration()
{
  Userinfo user;
  memset(&user, 0, sizeof(user));
  user.model = REG;
  cout<<"Please input username:";
  cin>>user.userName;
  //fgets(user.userName, BUFSIZE, stdin); 
  //user.userName[strlen(user.userName)-1] = '\0';

  cout<<"Please input password:";
  cin>>user.password;
  //fgets(user.password, BUFSIZE, stdin); 
  //user.password[strlen(user.password)-1] = '\0';
  
  int res = send(m_sockfd,&user,sizeof(user),0);
  if(res == -1)
  {
    perror("Reg Send error");
  } 

  char recv_buf[6] = {0};
  int nbytes= recv(m_sockfd, recv_buf, 5, 0);
  
  if (nbytes <= 0)
  {
    perror("In Function \"ClientRegistration\" : recv error");
    return ;
  }

  if (strncmp(recv_buf, "succe", 5) == 0)
  {
    printf("Register success\n");
  }
  else if (strncmp(recv_buf, "exist", 5) == 0)
  {
    printf("Username is exist\n");
  }

  close(m_sockfd);
}

//登录成功后的操作
void CClient::UserOperation()
{
  char command[BUFSIZE] = {0};
  char echobuf[BUFSIZE*10] = {0};
  bool running = true;
  int nbytes = 0;

  CreateDirectionText();
  MakeDirectionTree();
  close(m_sockfd);
  m_sockfd = -1;
  current_pos = file_tree.GetRootDirection();

  while (running)
  {
    
    //fgets(command, BUFSIZE, stdin);
    //command[strlen(command)-1] = '\0';
    //
    //gets(command);
    
    printf("[HerCloud]$");
    fflush(stdin);
    gets(command);
    
    if (strncmp(command, "exit", 4) == 0)
    {
      running = false;
    }
    else
    {
      ExecuteCommand(command);
    }

    memset(command, 0, BUFSIZE);
  }
}

//执行用户输入的命令
void CClient::ExecuteCommand(const char *command)
{
  char paramter[BUFSIZE] = {0};
  if (command == NULL )
  {
    perror("In Function \"ExecuteCommand\" : command can't be NULL");
    return ;
  }
  if (strncmp(command, "lsl", 3) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      lslocal(".");
    }
    else
    {
      lslocal(paramter);
    }
  }
  else if (strncmp(command, "ls", 2) == 0)
  {
    ls();
  }
  else if (strncmp(command, "cdl", 3) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      return ;
    }

    if (*paramter == '\0')
    {
      return ;
    }

    if (chdir(paramter) == -1)
    {
      printf("cd error\n");
    }
  }
  else if (strncmp(command, "cd", 2) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      return ;
    }

    if ( !(cd(paramter)) )
    {
      perror("In Function \" ExecuteCommand\" : direction dosen't exisit");
    }
  }
  else if (strncmp(command, "download", 8) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      return ;
    }

    if ( !(download(paramter)) )
    {
      printf("download failed\n"); 
    }
  }
  else if (strncmp(command, "upload", 6) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      printf("Lack paramter\n");
      return ;
    }

    if ( !(upload(paramter)) )
    {
      printf("upload failed\n");
    }
  }
  else if (strncmp(command, "snapshot", 8) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      printf("Lack paramter\n");
      return ;
    }

    if ( !(snapshot(paramter)) )
    {
      printf("make snapshot failed\n");
    }
  }
  else if (strncmp(command, "backto", 6) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      printf("Lack paramter\n");
      return ;
    }

    if ( !(backto(paramter)) )
    {
      printf("Backto snapshot failed\n");
    }
  }
  else if (strncmp(command, "rm", 2) == 0)
  {
    if ( !(GetParameter(command, paramter)) )
    {
      printf("Lack paramter\n");
      return ;
    }
    if ( !(remove(paramter)) )
    {
      printf("remove file failed\n");
    }
  }
  else if (strncmp(command, "snapls", 6) == 0)
  {
    if ( !(snaplist()) )
    {
      printf("Snap List Error\n");
      return ;
    }
  }
}

//创建目录树
void CClient::MakeDirectionTree()
{
  new (&file_tree) FileStruct(m_name);

  ifstream userfile;
  userfile.open("./.mydirinfo", ios::in);
  char buffer[BUFSIZE] = {0};

  while (!userfile.eof())
  {
    userfile.getline(buffer, BUFSIZE-1);
    if (buffer[0] == '\0') break;

    file_tree.AddFile(buffer);

    memset(buffer, 0, BUFSIZE);
  }
}

//创建目录树之前需要创建包含用户所有文件名的文本文件
void CClient::CreateDirectionText()
{
  int nbytes = 0;
  char recvBuff[BUFSIZE] = {0};

  int fd = open("./.mydirinfo", O_WRONLY|O_CREAT|O_TRUNC, 0666);  

  while ( (nbytes = recv(m_sockfd, recvBuff, 64, 0)) > 0)
  {
    int writeBytes = write(fd, recvBuff, nbytes);
    if (writeBytes != nbytes)
    {
      perror("write direction information error");
    }
    if (nbytes < 64)
    {
      break;
    }
  }

  close(fd);
}

//获取命令的参数
bool CClient::GetParameter(const char *command, char *arr)
{
  if (command == NULL || arr == NULL)
  {
    return false;
  }
  const char *ptr = command;
  const char *tail = NULL;
  int count = 0;

  while (*ptr++ != '\0')
  {
    if (*ptr == ' ')
    {
      ++count;
      if (count > 1) return false;

      strcpy(arr, ptr+1);
    }
  }

  if (count == 0)
  {
    return false;
  }

  return true;
}


bool CClient::UploadDirection(const char *file_name)
{
  
}





////////////客户端命令////////////////////////////
void CClient::ls()
{
  file_tree._ls(current_pos);
}

void CClient::lslocal(const char *pathname)
{
  DIR *dp = NULL;
  struct dirent *entry = NULL;
  struct stat stabuf;

  if ( (dp=opendir(pathname)) == NULL )
  {
    perror("Open directory failed");
    return ;
  }
  
  char current_dir[BUFSIZE] = {0};
  getcwd(current_dir, BUFSIZE);
  chdir(pathname);

  while ( (entry=readdir(dp)) != NULL )
  {
    lstat(entry->d_name, &stabuf);
    
    if ( strncmp(entry->d_name, ".", 1)==0 )
    {
      continue; 
    }
    if ( S_ISDIR(stabuf.st_mode) )
    {
      printf("\033[34;48m%s  \033[0m", entry->d_name);
    }
    else if ( access(entry->d_name, X_OK)==0 )
    {
      printf("\033[32;48m%s  \033[0m", entry->d_name);
    }
    else
    {
      printf("%s  ", entry->d_name);
    }   
  }

  chdir(current_dir);
  printf("\n");
  closedir(dp);
}
  
bool CClient::cd(const char *pathname)
{
  return file_tree._cd(current_pos, pathname);
}

//download 和 upload 目前一次只能操作一个文件,并且不能下载目录，有待优化
bool CClient::download(const char *file_name)
{
  char file_path[BUFSIZE] = {0};

  if ( !file_tree.GetPathByFileName(current_pos, file_name, file_path) )
  {
    perror("In Function \"download\" : file doesn't exisit");
    return false;
  }

  char requestBuf[BUFSIZE] = {0};
  strcpy(requestBuf, "GET ");
  strcat(requestBuf, file_path);

  //打开套接字，把路径发给服务器，服务器传送数据。
  CreateSocket(CTP);
  int nbytes = send(m_sockfd, requestBuf, strlen(requestBuf), 0);
  if (nbytes == -1)
  {
    perror("In Function \"download\": send error");
    return false;
  }
  close(m_sockfd);
  m_sockfd = -1;

  CreateSocket(DLP);
  memset(file_path, 0, BUFSIZE);

  char recvBuff[BUFSIZE] = {0};

  strcpy(file_path, "./cloud/");
  strcat(file_path, m_name);
  strcat(file_path, "/");
  strcat(file_path, file_name);

  int fd = open(file_path, O_WRONLY|O_CREAT, 0666); 

  while ( (nbytes = recv(m_loadfd, recvBuff, 64, 0)) > 0)
  {
    int writeBytes = write(fd, recvBuff, nbytes);
    if (writeBytes != nbytes)
    {
      perror("write direction information error");
      return false;
    }
    if (nbytes < 64)
    {
      break;
    }
  }

  close(fd);
  close(m_loadfd);
  m_loadfd = -1;
  return true;
}
bool CClient::upload(const char *file_name)
{
  if (file_name == NULL || *file_name == '\0')
  {
    perror("In Function \"upload\": file_name can't be NULL");
    return false;
  }

  char file_path[BUFSIZE] = {0};
  getcwd(file_path, BUFSIZE-1);
  strcat(file_path, "/");
  strcat(file_path, file_name);

  struct stat statbuf;

  lstat(file_path, &statbuf);

  if ( S_ISDIR(statbuf.st_mode) )
  {
    if ( UploadDirection(file_name) )
      return true;
    else
      return false;
  }

  int fd = open(file_path, O_RDONLY);
  if (fd == -1)
  {
    perror("In Function \"upload\": open file error");
    return false;
  }
  memset(file_path, 0, BUFSIZE);

  //获得该文件相对于用户根目录的路径
  file_tree.GetPathByFileName(current_pos, file_name, file_path);//file_path 是一个value-result参数

  char md5_path[BUFSIZE] = "./";
  unsigned int md5_src_code[4] = {0};
  //char md5_code[sizeof(unsigned int)*4] = {0};
  strcat(md5_path, file_name);

  MD5:: getMD5(md5_path, (unsigned long*)md5_src_code);//create MD5 code

  char requestBuf[BUFSIZE] = {0};
  strcpy(requestBuf, "PUT ");
  strcat(requestBuf, file_path);
  int send_bytes = strlen(requestBuf) + 1 + sizeof(md5_src_code);

  int *copy_pos = (int*)(requestBuf + (strlen(requestBuf) + 1));

  memcpy(copy_pos, md5_src_code, sizeof(md5_src_code));

  //open socket and connect to server, send request to server
  CreateSocket(CTP);
  int nbytes = send(m_sockfd, requestBuf, send_bytes, 0);
  if (nbytes == -1)
  {
    perror("In Function \"upload\": send error");
    return false;
  }


  struct stat statBuf;

  fstat(fd, &statBuf);

  char echo_buf[3] = {0};
  while (strncmp(echo_buf, "OK", 2) != 0)
  {
    CreateSocket(DLP);
    sendfile(m_loadfd, fd, NULL, statBuf.st_size);
    nbytes = recv(m_sockfd, echo_buf, 3, 0);//this MD5 code is recive from server
    if (nbytes < 0)
    {
      perror("upload error: recv echo buffer error");
      return false;
    }
  }

  file_tree.AddFile(file_path);

  close(m_sockfd);
  m_sockfd = -1;

  close(fd);

  close(m_loadfd);
  m_loadfd = -1;

  return true;
}

bool CClient::snapshot(const char *snapname)
{
  char requestBuf[BUFSIZE] = "SNAP ";
  strcat(requestBuf, m_name);
  strcat(requestBuf, " ");
  strcat(requestBuf, snapname);

  CreateSocket(CTP);
  int nbytes = send(m_sockfd, requestBuf, strlen(requestBuf), 0);
  if (nbytes == -1)
  {
    perror("In Function \"download\": send error");
    return false;
  }
  close(m_sockfd);
  m_sockfd = -1;
  return true;
}

bool CClient::backto(const char *snapname)
{
  char requestBuf[BUFSIZE] = "BACK ";
  strcat(requestBuf, m_name);
  strcat(requestBuf, " ");
  strcat(requestBuf, snapname);

  CreateSocket(CTP);
  int nbytes = send(m_sockfd, requestBuf, strlen(requestBuf), 0);
  if (nbytes == -1)
  {
    perror("In Function \"download\": send error");
    return false;
  }

  CreateDirectionText();
  close(m_sockfd);
  m_sockfd = -1;

  file_tree.DestroyFileTree();
  MakeDirectionTree();
  current_pos = file_tree.GetRootDirection();

  return true;
}

bool CClient::remove(const char *file_name)
{
  char file_path[BUFSIZE] = {0};
  if ( !file_tree.GetPathByFileName(current_pos, file_name, file_path) )
  {
    perror("In Function \"download\" : file doesn't exisit");
    return false;
  }

  char requestBuf[BUFSIZE] = "RM ";
  strcat(requestBuf, m_name);
  strcat(requestBuf, " ");
  strcat(requestBuf, file_path);

  //printf("%s\n", file_path);

  CreateSocket(CTP);
  int nbytes = send(m_sockfd, requestBuf, strlen(requestBuf), 0);
  if (nbytes == -1)
  {
    perror("In Function \"download\": send error");
    return false;
  }

  close(m_sockfd);
  m_sockfd = -1;

  if ( !(file_tree.RemoveFile(current_pos, file_name)) )
  {
    printf("Can't find file  ");
    return false;
  }

  return true;
}


bool CClient::snaplist()
{
  char requestBuf[BUFSIZE] = "SL ";
  strcat(requestBuf, m_name);

  CreateSocket(CTP);
  int nbytes = send(m_sockfd, requestBuf, strlen(requestBuf), 0);
  if (nbytes == -1)
  {
    perror("In Function \"snaplist\": send request error");
    return false;
  }

  char echoBuf[BUFSIZE] = {0};
  nbytes = recv(m_sockfd, echoBuf, BUFSIZE-1, 0);
  if (nbytes == -1)
  {
    perror("In Function \"snaplist\" : recv echo error");
    return false;
  }

  printf("%s\n", echoBuf);

  close(m_sockfd);
  m_sockfd = -1;
  return true;
}

/*
//这个获取参数写得太渣了！！！自己都看不下去！！！
bool CClient::GetLoadParameters(const char *command, char *parameter[BUFSIZE])
{
  if (command == NULL || parameter == NULL)
  {
    return false;
  }
  const char *head = command;
  const char *tail = head;
  int copy_size = 0;
  int i = 0;

  while (*tail != '\0')
  {
    if (*tail == ' ')
    {
      --tail;
      copy_size = tail - head;
      if (i > 9)
      {
        cerr<<"In Function \"GetLoadParameters\": to many paramters, must <=9"<<endl;
        return false;
      }
      //memset(parameter[i], 0, BUFSIZE);//暂时去掉，如果在这里有bug，需要加上这句话
      strncpy(parameter[i++], head, copy_size);
      ++tail;
      head = tail + 1;
    }
    ++tail;
  }

  strcpy(parameter, head);

  return true;
}
*/
