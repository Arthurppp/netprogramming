#ifndef __SQL_H__
#define __SQL_H__

#include <mysql/mysql.h>

#define QUERERR   -1
#define NOUSER    0
#define SUCCESS   1

class ConnMySQL
{
public:
  ConnMySQL();
  ~ConnMySQL();
public:
  int Query(const char *command, char *username, char *userpassword);
  bool Insert(const char *command);
private:
  MYSQL connection;
  MYSQL_RES *res;
  MYSQL_ROW row;
};

#endif //__SQL_H__
