#include "sql.h"
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

#define MYSQLUSRNAME   "root"
#define MYSQLUSRPASSWD "worinima"
#define DBNAME         "MyCloud"

ConnMySQL::ConnMySQL()
{
  //connection = mysql_init(NULL);
  mysql_init(&connection);
  /*
  if (connection == NULL)
  {
    perror("In Function \"ConnMySQL\" : Init of mysql failed");
    return;
  }
  */

  if ( !mysql_real_connect(&connection, "localhost", MYSQLUSRNAME, MYSQLUSRPASSWD, DBNAME, 0, NULL, 0) )
  {
    perror("In Function \"ConnMySQL\" : Connect to mysql error");
    return ;
  }
}
ConnMySQL::~ConnMySQL()
{
  //if (connection != NULL)
  //{
    mysql_close(&connection);
  //}
}

int ConnMySQL::Query(const char *command, char *username, char *userpassword)
{
  int ret = mysql_query(&connection, command);

  if (ret != 0)
  {
    printf("In Function \"Query\" : query error : %s\n", mysql_error(&connection));
    return QUERERR;
  }

  res = mysql_store_result(&connection);

  if (mysql_num_rows(res) == 0)
  {
    return NOUSER;
  }

  row = mysql_fetch_row(res);

  //printf("%s\n", row[0]);

  //printf("--------------------------------------------");

  //printf("%s\n", row[1]);
  if (username != NULL && userpassword != NULL)
  {
    strcpy(username, row[0]);
    strcpy(userpassword, row[1]);
  }

  mysql_free_result(res);
  return SUCCESS;
}

bool ConnMySQL::Insert(const char *command)
{
  int ret = mysql_query(&connection, command);

  if (ret != 0)
  {
    printf("In Function \"Insert\" : query error : %s\n", mysql_error(&connection));
    return false;
  }

  return true;
}
