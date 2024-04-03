#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>
#include<errno.h>
#include<netinet/in.h>

#include<pthread.h>
#include<unistd.h>

#include<sys/epoll.h>
#include<arpa/inet.h>


#define BUFFER_LENGTH		1024





int init_localsocket(char *ip, int port) {

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		perror("socket\n");
		return -1;
	}

	struct sockaddr_in localaddr;
	memset(&localaddr, 0, sizeof(struct sockaddr_in));

	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(port);
	localaddr.sin_addr.s_addr = inet_addr(ip);

	if(-1 == bind(sockfd, (struct sockaddr*)&localaddr, sizeof(struct sockaddr_in))) {
		perror("bind");
		return -2;
	};

	return sockfd;
	
}


int conn_client(char *ip, int port, int sockfd) {

	struct sockaddr_in clientaddr;
	memset(&clientaddr, 0, sizeof(struct sockaddr_in));

	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(port);
	clientaddr.sin_addr.s_addr = inet_addr(ip);

	if (connect(sockfd, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_in)) < 0) {
		return -1;
	}

	return 0;
	
}


void *client_thread(void *arg) {

	int clientfd = *(int *)arg;

	while(1) {

		printf("client > ");
		char buffer[BUFFER_LENGTH];
		scanf("%s", buffer);
		if(strcmp(buffer, "quit") == 0) {
			break;
		}
		int len = strlen(buffer);
		printf("len: %d, buffer: %s\n", len, buffer);
		send(clientfd, buffer, len, 0);

	}
		
}



int main(int argc, char **argv) {
	if (argc <= 2) {
		printf("Usage: %s ip port\n", argv[0]);
		exit(0);
	}

	char *ip = argv[1];
	int port = atoi(argv[2]);
	
	int sockfd = init_localsocket("0.0.0.0", 8000);

	while(1) {
		int ret = conn_client(ip, port, sockfd);
		if(ret < 0){
			usleep(1);
			continue;
		}
		break;
	}
	printf("connect successful\n");


	pthread_t thid;
	pthread_create(&thid, NULL, client_thread, &sockfd);

	int epfd = epoll_create(1);
	struct epoll_event ev;
	
	ev.data.fd = sockfd;
	ev.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

	struct epoll_event events[1] = {0};
	
	while(1) {

		int nready = epoll_wait(epfd, events, 1, -1);

		for(int i=0; i<nready; i++) {

			int connfd = events[i].data.fd;
			if(events[i].events & EPOLLIN){
				char buffer[BUFFER_LENGTH] = {0};
				int len = recv(connfd, buffer, BUFFER_LENGTH, 0);
				if(len == 0) {
					printf("disconnect\n");
					epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
					close(i);
					continue;
				}

				printf("recv --> len: %d, buffer: %s\n", len, buffer);
			}

		}
	}
	
}












