/*
 *title:server.c
 *content:server part
 *start time:2015.10.10 
 *end time:2015.10.27
 */

#include"i.h"

int user_list_fd;

int init()
{
	i_init();
	
	user_list_fd = i_open("./user_list", O_RDWR|O_CREAT);

	struct user usr;
	/* init the user list file's fist user to 0*/
	memset((struct user*)&usr, '\0', sizeof(struct user));
	i_lseek(user_list_fd, 0, SEEK_SET);
	i_write(user_list_fd, (char*)&usr, USR_LEN);

	/*bind the struct sockaddr_in server to the sockfd*/
	i_bind(sockfd, (struct sockaddr*)&server, ADDR_LEN);

	struct chat_history apple;
	
	memset(&apple, 0, HSTR_LEN);
	i_lseek(mainfd, 0, SEEK_SET);
	i_write(mainfd, &apple, HSTR_LEN);
	i_lseek(mainfd, -HSTR_LEN, SEEK_END);
	i_read(mainfd, &apple, HSTR_LEN);
	count = apple.count;

	return 0;
}

int send_msg(struct msg *msg_recv, struct sockaddr *addr)
{
	int i;
	struct user usr;

	/*a common essage come*/
	printf("a ordinar message come!\n");

	i = msg_recv->id_to;
	i_lseek(user_list_fd, i*USR_LEN, SEEK_SET);
	i_read(user_list_fd, &usr, USR_LEN);
	strncpy(msg_recv->append, usr.name, 10);

	i_sendto(sockfd, msg_recv, MSG_LEN, 0, &(usr.user_addr), ADDR_LEN);

	printf("id%d send a message to id%d sucess!\n", msg_recv->id_from, msg_recv->id_to);

	return 0;
}

int check_login(struct msg *msg_recv, struct sockaddr *addr)
{
	int i = msg_recv->id_from;
	struct user usr;
	
	/*a login request*/
	printf("a login request come!\n");

	/*get the id's information*/
	i_lseek(user_list_fd, i*USR_LEN, SEEK_SET);
	i_read(user_list_fd, &usr, USR_LEN);

	int n;
	n = strcmp(usr.passwd, msg_recv->content);

	/*如果验证成功，则发送成功消息*/
	if(n == 0)
	{
		/* save user new address */
		i_lseek(user_list_fd, -USR_LEN, SEEK_CUR);
		usr.user_addr = *addr;
		i_write(user_list_fd, &usr, USR_LEN);
		/* tell user pass */
		i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0,&(usr.user_addr), ADDR_LEN);
	}
	else
	{
		/*出错的话respond*/
		if(0 != n)
		{
			printf("id%d login error.\n", i);
			memset(msg_recv->content, 0, CNTNT_LEN);
			msg_recv->flag = -1;
			i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, &(usr.user_addr), ADDR_LEN);
		}
		return 1;
	}
	
	printf("id%d login sucess!\n", i);

	return 0;
}

int reg_user(struct msg *msg_recv, struct sockaddr *addr)
{
	struct user usr;
	
	printf("a regit request come:\n");
	
	/* find the last user and have the please to add a new user*/
	int n;
	i_lseek(user_list_fd, -USR_LEN, SEEK_END);
	i_read(user_list_fd, &usr, USR_LEN);

	/*把新用户的信息赋值到usr然后填入到usr list file中*/
	const char *name;
	const char *passwd;

	name = &(msg_recv->content[0]);
	passwd = &(msg_recv->content[10]);
	strcpy(usr.name, name);
	strcpy(usr.passwd, passwd);
	memcpy(&(usr.user_addr), addr, ADDR_LEN);

	usr.id = (usr.id + 1);
	i_lseek(user_list_fd, 0, SEEK_END);
	i_write(user_list_fd, &usr, USR_LEN);

	msg_recv->id_from = usr.id;

	/* regist to the user list then tell theuser reg success*/
	i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);

	printf("id%d regist success!\n", usr.id);

	return 0;
}

/*ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
 *                const struct sockaddr *dest_addr, socklen_t addrlen);
 *ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
 *                 struct sockaddr *src_addr, socklen_t *addrlen);
 *注意细微区别
*/

int msg_cntl()
{
	struct msg msg_recv;
	struct sockaddr addr_recv;

	printf("begin listen input...\n");
	int size = ADDR_LEN;

	for(;;)
	{
		memset(&msg_recv, 0, MSG_LEN);
		i_recvfrom(sockfd, &msg_recv, sizeof(struct msg), 0, &addr_recv, &size);
		printf("message received...\n");
		
		i_saveto_chat(&msg_recv);

		switch(msg_recv.flag)
		{
			case 1 :
				send_msg(&msg_recv, (struct sockaddr*)&addr_recv);
				break;
			case 2 :
				check_login(&msg_recv, (struct sockaddr*)&addr_recv);
				break;
			case 3 :
				reg_user(&msg_recv, (struct sockaddr*)&addr_recv);
			default :
				break;
		}
	}

	return 0;
}

int exit_sys()
{
	close(sockfd);
	close(mainfd);
	close(user_list_fd);
	printf("exit system");
	kill(0, SIGABRT);

	exit(0);
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
	printf("****chat*history****");
	printf("(n - nextpage; p - prepage; q - quit)\n");

	int page_num;
	int remains;
	int berry = get_page_size();

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

	int i = -1;

	while(1)
	{
		char flag;

		if( (berry + i*8) >= 0)
		{
			printf("( %d ~ %d )\n", (berry + i*8), (berry + (i+1)*8));

			i_print_history(PRT_LEN, i);

			printf("@@@\n");
			while('\n' == (flag = getchar()))
			{
			}

			switch(flag)
			{
				case 'p' :
					i--;
					break;
				case 'n' :
					i++;
					break;
				case 'q' :
					return 0;
				default :
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
			printf("( 1 ~ %d )\n", remains);
			
			i_print_history(remains, 0);

			printf("######## over ########\n");
			
			return 0;
		}
	}

	return 0;
}

int menu()
{
	sleep(1);

	printf("----------help----menu---------\n");
        printf("\t r--report to user\n");
        printf("\t c--chat history\n");
        printf("\t h--help menu\n");
        printf("\t e--exit the system\n");
        printf("----------help_menu---------\n");
 
        int command = 0;
 
        printf("input command>");
        command = getchar();
        
	switch(command)
    	{
 
        	case 'c':
            		read_chat_history();
            		break;
        	case 'e':
	            	exit_sys();
            		break;
        	case 'r':
            		//report();
            		//break;
        	default :
            		menu();
            		break;
    	}	
    	
	getchar();
     
    	return 0;
}

int main()
{
	init();
	pid_t pid;

	switch(pid = fork())
	{
		case -1:
			perror("fork error\n");
			exit(1);
			break;
		case 0:
			msg_cntl();
			break;
		default:
			menu();
			break;
	}
	
	return 0;
}
