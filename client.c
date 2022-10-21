#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];
char password[32];
char group[32];

void str_overwrite_stdout()
{
	printf("%s", "> ");
	fflush(stdout);
}
void logo()
{

	char lg[29] = "#CHATROOM_CLIENT#";

	printf("\e[1m");
	printf("\e[96m%s\n", lg);
	printf("\e[21m\e[0m");
}

void str_trim_lf(char *arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{ // trim \n
		if (arr[i] == '\n')
		{
			arr[i] = '\0';
			break;
		}
	}
}

void catch_ctrl_c_and_exit(int sig)
{
	flag = 1;
}

void send_msg_handler()
{
	char messagee[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

	while (1)
	{

		str_overwrite_stdout();
		fgets(messagee, LENGTH, stdin);
		str_trim_lf(messagee, LENGTH);

		if (strcmp(messagee, "/exit") == 0)
		{
			sprintf(buffer, "%s \n", messagee);
			send(sockfd, buffer, strlen(buffer), 0);
			break;
		}
		else if (strcmp(messagee, "/close") == 0)
		{
			sprintf(buffer, "%s %s\n", messagee, name);
			send(sockfd, buffer, strlen(buffer), 0);
		}
		else if (strncmp(messagee, "/msg", 4) == 0)
		{
			sprintf(buffer, "%s \n", messagee);
			send(sockfd, buffer, strlen(buffer), 0);
		}
		else if (strncmp(messagee, "/m", 2) == 0)
		{
			sprintf(buffer, "%s \n", messagee);
			send(sockfd, buffer, strlen(buffer), 0);
		}
		else if (strncmp(messagee, "/chathistory", 12) == 0)
		{
			sprintf(buffer, "%s \n", messagee);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(messagee, LENGTH);
		bzero(buffer, LENGTH + 32);
	}
	catch_ctrl_c_and_exit(2);
}

void recv_msg_handler()
{
	char message[LENGTH] = {};
	while (1)
	{
		int receive = recv(sockfd, message, LENGTH, 0);
		if (receive > 0)
		{
			printf("%s", message);
			str_overwrite_stdout();
		}
		else if (receive == 0)
		{
			break;
		}
		else
		{
			// -1
		}
		memset(message, 0, sizeof(message));
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}
	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("Please enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	if (strlen(name) > 32 || strlen(name) < 2)
	{
		printf("Name must be less than 30 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	printf("Please enter your password: ");
	fgets(password, 32, stdin);
	str_trim_lf(password, strlen(password));

	if (strlen(password) > 32 || strlen(password) < 2)
	{
		printf("Name must be less than 30 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	printf("Please enter your group: ");
	fgets(group, 32, stdin);
	str_trim_lf(group, strlen(group));

	if (strlen(group) > 32 || strlen(group) < 2)
	{
		printf("GroupName must be less than 30 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	// Connect to Server
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1)
	{
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	send(sockfd, name, 32, 0);
	send(sockfd, password, 32, 0);
	send(sockfd, group, 32, 0);

	logo();

	pthread_t send_msg_thread;
	if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1)
	{
		if (flag)
		{
			printf("\033[1;31m");
			printf("THANKS FOR USING MY SERVICE\n");
			printf("\033[0m;");
			break;
		}
	}

	close(sockfd);

	return EXIT_SUCCESS;
}
