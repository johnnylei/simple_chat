#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>


#define set_nonblocking(fd) do { \
	int option = fcntl(fd, F_GETFL); \
	fcntl(fd, F_SETFL, option | O_NONBLOCK); \
} while(0);

#define client_fd_init(arr, len) do { \
	for (int i = 0; i < len; i++) { \
		arr[i] = 0; \
	} \
} while(0);

#define MAX_EVENT_NUMBER 1024
#define MAX_CLIENT_FD_LEN 1024
#define BUFF_SIZE 1024

typedef struct accept_client_param {
	int server_fd;
	int epfd;
	int * client_fd;
} accept_client_param_t;

int last_client_fd_index (int *arr, int len) {
	int i;
	for (i = 0; i < len; i++) {
		if (arr[i] == 0) {
			break;
		}
	}

	return i;
}


void * accept_client(void * context) {
	accept_client_param_t * param = (accept_client_param_t *)context;
	int client_fd;

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int client_fd_len;
	while (client_fd = accept(param->server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) {
		struct epoll_event read_event;
		read_event.data.fd = client_fd;
		read_event.events = EPOLLIN | EPOLLET; 
		epoll_ctl(param->epfd, EPOLL_CTL_ADD, client_fd, &read_event);
		set_nonblocking(client_fd);
		int index = last_client_fd_index(param->client_fd, MAX_CLIENT_FD_LEN);
		param->client_fd[index] = client_fd;
	}
}

int main() {
	struct sockaddr_in server_address;
	server_address.sin_port = ntohs(12345);
	server_address.sin_family = AF_INET;
	inet_aton("127.0.0.1", &server_address.sin_addr);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(server_fd >= 0);

	int ret = bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
	assert(ret >= 0);

	ret = listen(server_fd, 5);
	assert(ret >= 0);

	int epfd = epoll_create(5);
	assert(epfd >= 0);

	pthread_t handle;
	accept_client_param_t param;
	param.server_fd = server_fd;
	param.epfd = epfd;
	int client_fds[MAX_CLIENT_FD_LEN];
	client_fd_init(client_fds, MAX_CLIENT_FD_LEN);
	param.client_fd = client_fds;

	ret = pthread_create(&handle, NULL, accept_client, &param);
	assert(ret == 0);

	struct epoll_event events[MAX_EVENT_NUMBER];
	char read_buff[BUFF_SIZE];
	char send_buff[] = "this is server\n";

	int active_len;
	while(1) {
		active_len = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
		assert(active_len >= 0);

		for (int i = 0; i < active_len; i++) {
			int client_fd = events[i].data.fd;

			if (events[i].events & EPOLLIN) {
				memset(read_buff, '\0', BUFF_SIZE);
				ret = recv(client_fd, read_buff, BUFF_SIZE - 1, 0);
				if (ret < 0 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
					break;
				}
				
				assert(ret >= 0);

				int client_fd_len = last_client_fd_index(client_fds, MAX_CLIENT_FD_LEN);
				for (int j = 0; j < client_fd_len; j++) {
					if (client_fds[j] == client_fd) {
						continue;
					}

					printf("readbuff=%s\n", read_buff);
					ret = send(client_fds[j], read_buff, strlen(read_buff), 0);
					assert(ret >= 0);
				}
			}
		}
	}

	close(server_fd);
}
