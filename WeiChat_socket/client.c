/*
 *title: client.c
 *start_time: 2015.10.10
 *end_time:  2015.11.1
 */

#include"i.h"

#define START_PORT 8089

struct sockaddr_in my_addr;
int my_id;

int my_log();  //declare function

int i_send_msg()
{
	int id;
	struct msg the_msg;
	char end = '@';

	printf("input recver id:");
	scanf("%d", &id);
	getchar();
	printf("\ninput content:");
	i_input(the_msg.content);

	char flag = 'y';
	
 	if(1)
	{
		the_msg.flag = 1;
		the_msg.id_from = my_id;
		the_msg.id_to = id;

		i_sendto(sockfd, &the_msg, sizeof(struct msg), 0, (struct sockaddr*)&server, sizeof(struct sockaddr));

		i_saveto_chat(&the_msg);/* save to history */

		printf("send to id:%d success.\n", my_id);
		return 0;
	}
	else
	{
		return 1;
	}
	
	return 0;
}

int reply()
{
	return 0;
}

int send_file()
{
	return 0;
}

int init()
{
	struct ifreq req;
	struct sockaddr_in *host;
	int port;

	i_init();
	/* init user addr */
	memset(&my_addr, 0,sizeof(struct sockaddr));
	my_addr.sin_family = AF_INET;
	strcpy(req.ifr_name, "lo");//interface name
	if( ioctl(sockfd, SIOCGIFADDR, &req) < 0 )//获取接口地址
	{
		perror("get local ip error");
		exit(1);
	}

	host = (struct sockaddr_in*)&(req.ifr_addr);//sockaddr -> sockaddr_in
	printf("ip: %s\n", inet_ntoa(host->sin_addr));//网络字节序转换成点分十进制ip
	
	memcpy(&my_addr, (struct sockaddr_in*)&(req.ifr_addr), sizeof(struct sockaddr_in));
	
	port = START_PORT;

	do
	{
		port++;
		my_addr.sin_port = htons(port);
		bind(sockfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr));
	}
	while( errno == EADDRINUSE );//地址使用错误

	struct chat_history apple;

	memset(&apple, 'b', HSTR_LEN);
	i_lseek(mainfd, 0, SEEK_SET);
	apple.count = 0;
	i_write(mainfd, &apple, HSTR_LEN);
	i_lseek(mainfd, -HSTR_LEN, SEEK_END);
	i_read(mainfd, &apple, HSTR_LEN);

	printf("port:%d\n", port);
	printf("init success!!!\n");

	return 0;
}

int get_page_size()
{
	struct chat_history page_size_reader;

	i_lseek(mainfd, -HSTR_LEN, SEEK_END);
	i_read(mainfd, &page_size_reader, HSTR_LEN);

	return page_size_reader.count;
}

int read_chat_history()
{
	printf("******** char history ********\n");
	printf("( n - next; p - pre; q - quit )\n");

	int page_num;//显示的页数
	int remains;//最后一页显示的条数
	int berry = get_page_size();//聊天记录条数

	page_num = berry/8;
	remains = berry%8;

	if(remains != 0)
	{
		page_num++;
	}
	else
	{
		page_num = page_num;
	}

	printf("there are %d page total %d items", page_num, berry);

	int i = -1;

	while(1)
	{
		char flag;

		if((berry+i*8) >= 0)
		{
			printf("(%d~%d)\n", (berry+i*8),(berry+(i+1)*8));
			
			i_print_history(PRT_LEN, i);

			printf("@@@\n");
			while('\n' == (flag = getchar()))
			{
			}

			switch(flag)
			{
				case 'p':
					i--;
					break;
				case 'n':
					i++;
					break;
				case 'q':
					return 0;
				default:
					break;
			}
			if(i >= 0)
			{
				printf("have at the end!\n");
				printf("return to menu!\n");
			}
		}
		else
		{
			printf("(1~%d)\n", remains);
			
			i_print_history(remains, 0);

			printf("######### over #########\n");

			return 0;
		}
	}

	return 0;
}

void exit_sys()
{
	close(sockfd);
	close(mainfd);
	kill(0, SIGABRT);

	exit(0);
}

/*start menu*/
int print_menu()
{
    	printf("\n--------------help--menu----------------\n");
    	printf("\t h--help munu\n");
    	printf("\t s--send message\n");
    	printf("\t r--reply to\n");
    	printf("\t c--chat history\n");
    	printf("\t f--send files\n");
    	printf("\t e--exit the system\n");
    	printf("----------------help--menu----------------\n");
}

int get_input(char *command)
{
	printf(">");
	scanf("%c", command);

	return 1;
}

int menu()
{
	/* to avoid the ouput at mixed with the sub process */
	sleep(1);

	print_menu();

	char command;

	while(1 == get_input(&command))
	{
		switch(command)
		{
			case 'h':
				print_menu();
				break;
			case 's':
				i_send_msg();
				break;
			case 'r':
				reply();
				break;
			case 'c':
				read_chat_history;
				break;
			case 'e':
				exit_sys();
				break;
			default:
				printf(">");
				break;
		}
	}
	
	return 0;
}
/*end menu*/



/* start:message control: send_msg and recv_msg */
int ordinary_msg_recv(struct msg *pmsg)
{
	char time_info[25];
	char end_symble;
	end_symble = '&';

	/*handle tne msg*/
	printf("Message: from %s(id%d) to U:\n", pmsg->append, pmsg->id_from);
	i_print(pmsg->content, MSG_LEN);
	printf("\n\t%s", i_get_time());

	return 0;
}

int file_msg_recv(struct msg *pmsg)
{
}

int handle_msg(struct msg *pmsg)
{
	if(pmsg->flag == 1)
	{
		ordinary_msg_recv(pmsg);
		return 0;
	}
	else if(pmsg->flag >= 4)
	{
		file_msg_recv(pmsg);
		return 0;
	}
	return 0;
}

int listen_msg()
{
	struct msg msg_recv;
	struct sockaddr addr_recv;
	int len = ADDR_LEN;

	printf("begin listen...\n");

	for( ; ; )
	{
		i_recvfrom(sockfd, &msg_recv, MSG_LEN, 0, &addr_recv, &len);
		
		i_saveto_chat(&msg_recv);//存记录

		ordinary_msg_recv(&msg_recv);
	}
}
/*end: message control*/

/*start: log process :login and regist */
int login()
{
	/*input id*/
	printf("*****login>>\n");
	printf("id:");
	scanf("%d", &my_id);
	/*input password*/
	char passwd[15];
	printf("\npassword(less 15 char):");
	scanf("%s", passwd);
	getchar();

	/*send login information*/
	struct msg log_msg;

	memset(&log_msg, 0,MSG_LEN);
	log_msg.flag = 2;
	log_msg.id_from = my_id;
	log_msg.id_to = 0;
	strncpy(log_msg.content, passwd, 15);

	i_saveto_chat(&log_msg);//保存到记录

	i_sendto(sockfd, (struct msg*)&log_msg, MSG_LEN, 0, (struct sockaddr*)&server, sizeof(struct sockaddr));

	/*after input msg, wait for server respond*/
	struct sockaddr in_addr;
	int len = ADDR_LEN;
	i_recvfrom(sockfd, (struct msg*)&log_msg, MSG_LEN, 0, &in_addr, &len);
	if(2 == log_msg.flag)
	{
		printf("login success\n");
		return 0;
	}
	else
	{
		printf("login error:%s\n", log_msg.content);
		printf("please relog...\n");
		menu();
	}

	return 0;
}

int regist()
{
	printf("*****regist>>\n");
	/* input char name */
	char name[10];

	memset(name, 0, 10);
	printf("input your chat name(less 8 char):");
	scanf("%s", name);

	//name[9] = ';'; 
	/*input password*/
	char passwd[15];

	memset(passwd, 0, 15);
	printf("\ninput your password(less 14 char):");
	scanf("%s", passwd);

	/*send regist information*/
	struct msg reg_msg;

	memset(&reg_msg, 0, MSG_LEN);
	reg_msg.flag = 3;
	reg_msg.id_from = 0;
	reg_msg.id_to = 0;
	memset(reg_msg.content, 0, CNTNT_LEN);
	strncpy(reg_msg.content, name, 10);
	strncpy(&(reg_msg.content[10]), passwd, 15);
	reg_msg.content[25] = '\n';

	i_saveto_chat(&reg_msg); /* save to history*/
	
	/* send regist information to server */
	i_sendto(sockfd, (struct msg*)&reg_msg, sizeof(struct msg), 0, (struct sockaddr*)&server, ADDR_LEN);
	/* after input msg, wait for server respond */
	printf("waiting for server reply...\n");

	struct sockaddr in_addr;
	struct msg msg_back;
	int len = ADDR_LEN;

	memset(&in_addr, 0, ADDR_LEN);
	memset(&msg_back, 0, MSG_LEN);
	i_recvfrom(sockfd, (struct msg*)&msg_back, MSG_LEN, 0, &in_addr, &len);

	/* check whether pass */
	if(3 != msg_back.flag)
	{
		printf("error: %s \n", msg_back.content);
		exit(1);
	}
	else
	{
		my_id = msg_back.id_to;
		printf("congratulations!you have registted id %s(id %d) successfully\n",msg_back.content, msg_back.id_to);
		
		login();
	}

	return 0;
}

int my_log()
{
	/* choose login or regist */
	char flag;
	printf("are you want login or regist(l/r)\n");
	scanf("%c", &flag);
	getchar();

	switch(flag)
	{
		case 'l':
			login();
			break;
		case 'r':
			regist();
			break;
		default:
			printf("error input\n");
			my_log();
			break;
	}
	return 0;
}
/* end: log */


int main()
{
	init();
	printf("\n**********welcome!**********\n");
	my_log();

	pid_t pid;

	switch(pid = fork())
	{
		case -1:
			perror("fork error!\n");
			exit(1);
			break;
		case 0:
			listen_msg();
			break;
		default :
			menu();
			break;
	}

	return 0;
}
