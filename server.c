#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

/* Client structure */
struct group_t
{
	char groupname[32];
};

typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	char password[32];
	struct group_t group;

} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout()
{
	printf("\r%s", "> ");
	fflush(stdout);
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

void print_client_addr(struct sockaddr_in addr)
{
	printf("%d.%d.%d.%d",
		   addr.sin_addr.s_addr & 0xff,
		   (addr.sin_addr.s_addr & 0xff00) >> 8,
		   (addr.sin_addr.s_addr & 0xff0000) >> 16,
		   (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}
int free_space(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clients[i])
		{
			clients[i] = cl;
			return i;
		}
	}
	return 0;

	pthread_mutex_unlock(&clients_mutex);
}
/* Strip CRLF */
void strip_newline(char *s)
{
	while (*s != '\0')
	{
		if (*s == '\r' || *s == '\n')
		{
			*s = '\0';
		}
		s++;
	}
}

/* Remove clients to queue */
void queue_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->uid == uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all group members */
void send_message(char *s, int uid, char *groupname)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{

			if (clients[i]->uid != uid && (strcmp(clients[i]->group.groupname, groupname) == 0))
			{
				if (write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void old_msg(int uid, char *groupname)
{
	pthread_mutex_lock(&clients_mutex);
	FILE *fptr;
	//char *c;
	char line[128];

	fptr = fopen(groupname, "r");
	if (fptr == NULL)
	{
		printf("Cannot open file \n");
		exit(0);
	}
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i])
		{
			if (clients[i]->uid == uid)
			{

				while (fgets(line, sizeof line, fptr) != NULL)
				{

					if (write(clients[i]->sockfd, line, strlen(line)) < 0)
					{
						perror("ERROR: write to descriptor failed");
						break;
					}
				}

				fclose(fptr);
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

int file_exists()
{

	if (access("contacts.txt", F_OK) != -1)
	{
		return 1;
	}
	else
		return 0;
}

//close all the sockets before exiting
void stop_group(char *groupname, char *username, char *password, int uid)
{
	pthread_mutex_lock(&clients_mutex);
	FILE *fptr1;
	char name[32];
	char pass[32];
	char group[32];

	if (file_exists() == 1)
	{
		fptr1 = fopen("contacts.txt", "r+");

		int j;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			fscanf(fptr1, "%s\n%s\n%s\n", name, pass, group);
			str_trim_lf(group, strlen(group));

			if (strcmp(groupname, group) == 0)
			{
				j = i;
				break;
			}
		}
		fclose(fptr1);

		fptr1 = fopen("contacts.txt", "r+");

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			fscanf(fptr1, "%s\n%s\n%s\n", name, pass, group);

			//checks if user is the admin of the group
			if (strcmp(username, name) == 0 && strcmp(password, pass) == 0 && strcmp(groupname, group) == 0 && j == i)
			{
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (clients[i])
					{
						if (strcmp(clients[i]->group.groupname, groupname) == 0)
						{

							clients[i] = NULL;
							//close(clients[i]->sockfd);
						}
					}
				}
			}
		}

		fclose(fptr1);
	}

	pthread_mutex_unlock(&clients_mutex);
}
void logo()
{

	char lg[29] = "#CHATROOM_SERVER#";

	printf("\e[5m\e[1m");
	printf("\e[96m%s\n", lg);
	printf("\e[25m\e[21m\e[0m");
}

/* Send message to a group member */
void send_message_to_user(char *s, int uid, char *groupname, char *username)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->uid != uid && (strcmp(clients[i]->group.groupname, groupname) == 0) && (strcmp(clients[i]->name, username) == 0))
			{
				if (write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Handle all communication with the client */
void *handle_client(void *arg)
{
	char buff_out[BUFFER_SZ];
	char name[32];
	char password[32];
	char group[32];
	int leave_flag = 0;
	char tempname[32];
	char temppass[32];
	char tempgroup[32];

	cli_count++;
	client_t *cli = (client_t *)arg;
	FILE *fptr;

	//reads and saves contacts to arrays
	char name1[32];
	char password1[32];
	char group1[32];
	FILE *fptr1;

	char offname[MAX_CLIENTS][32];
	char offpass[MAX_CLIENTS][32];
	char offgroup[MAX_CLIENTS][32];

	if (file_exists() == 1)
	{

		int j = 0;
		fptr1 = fopen("contacts.txt", "r");
		while (fscanf(fptr1, "%s\n%s\n%s\n", name1, password1, group1) != EOF)
		{

			strcpy(offname[j], name1);
			strcpy(offpass[j], password1);
			strcpy(offgroup[j], group1);
			j++;
		}
		fclose(fptr1);
	}

	fptr = fopen("contacts.txt", "a");
	if (fptr == NULL)
	{
		printf("Opening file error!");
		exit(1);
	}

	// Name
	if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1)
	{
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	}
	else
	{
		strcpy(tempname, name);
	}

	bzero(buff_out, BUFFER_SZ);

	// password
	if (recv(cli->sockfd, password, 32, 0) <= 0 || strlen(password) < 2 || strlen(password) >= 32 - 1)
	{
		printf("Didn't enter the password.\n");
		leave_flag = 1;
	}
	else
	{
		strcpy(temppass, password);
	}

	bzero(buff_out, BUFFER_SZ);

	// group
	if (recv(cli->sockfd, group, 32, 0) <= 0 || strlen(group) < 2 || strlen(group) >= 32 - 1)
	{
		printf("Didn't enter the group name.\n");
		leave_flag = 1;
	}
	else
	{
		strcpy(tempgroup, group);
	}

	// checks if a user is already saved in server
	int x = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{

		if (strcmp(offname[i], tempname) == 0 && strcmp(offpass[i], temppass) == 0 && strcmp(offgroup[i], tempgroup) == 0)
		{
			strcpy(cli->name, tempname);
			strcpy(cli->password, temppass);
			strcpy(cli->group.groupname, tempgroup);
			sprintf(buff_out, "%s | %s  has joined again\n", cli->name, cli->group.groupname);
			send_message(buff_out, cli->uid, cli->group.groupname);
			bzero(buff_out, BUFFER_SZ);
			x = 1;
		}
	}
	if (x == 0)
	{
		strcpy(cli->name, tempname);
		strcpy(cli->password, temppass);
		strcpy(cli->group.groupname, tempgroup);
		sprintf(buff_out, "%s | %s  has joined for the first time\n", cli->name, cli->group.groupname);
		send_message(buff_out, cli->uid, cli->group.groupname);
		bzero(buff_out, BUFFER_SZ);
		fprintf(fptr, "%s\n%s\n%s\n", cli->name, cli->password, cli->group.groupname);
		fclose(fptr);
	}

	char buff_in[BUFFER_SZ / 2];
	char *param;
	char *data;
	char *toUser;

	while (1)
	{

		if (leave_flag)
		{
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		FILE *fptr;

		strcpy(buff_in, buff_out);
		param = strtok(buff_in, " ");

		if (receive > 0)
		{

			if (strcmp(param, "/msg") == 0)
			{
				toUser = strtok(NULL, " ");
				data = strtok(NULL, "|");
				fptr = fopen(cli->group.groupname, "a");
				fprintf(fptr, "([%s|%s]-> [%s]) : %s\n", cli->name, cli->group.groupname, toUser, data);
				fclose(fptr);
				send_message_to_user(data, cli->uid, cli->group.groupname, toUser);
				printf("PRIVATE GROUP MESSAGE (%s)(%s -> %s) :%s \n", cli->group.groupname, cli->name, toUser, data);
			}
			else if (strcmp(param, "/m") == 0)
			{
				data = strtok(NULL, "|");
				fptr = fopen(cli->group.groupname, "a");
				fprintf(fptr, "([%s|%s]-> ALL) : %s\n", cli->name, cli->group.groupname, data);
				fclose(fptr);
				send_message(data, cli->uid, cli->group.groupname);
				str_trim_lf(buff_out, strlen(buff_out));
				printf("GROUP MESSAGE (%s) from (%s) :%s \n", cli->group.groupname, cli->name, data);
			}
			else if (strcmp(param, "/chathistory") == 0)
			{
				printf("Shows all messages of group [%s] to user [%s] \n", cli->group.groupname, cli->name);
				old_msg(cli->uid, cli->group.groupname);
			}
			else if (strcmp(param, "/close") == 0)
			{
				stop_group(cli->group.groupname, cli->name, cli->password, cli->uid);
			}
			else if (strcmp(param, "/exit") == 0)
			{
				sprintf(buff_out, "%s has left\n", cli->name);
				printf("%s", buff_out);
				send_message(buff_out, cli->uid, cli->group.groupname);
				leave_flag = 1;
			}
		}
		else
		{
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

	/* Delete client from queue and yield thread */
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
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
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	/* Socket settings */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	/* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
	{
		perror("ERROR: setsockopt failed");
		return EXIT_FAILURE;
	}

	/* Bind */
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	/* Listen */
	if (listen(listenfd, 10) < 0)
	{
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	logo();

	while (1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if ((cli_count + 1) == MAX_CLIENTS)
		{
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void *)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}
