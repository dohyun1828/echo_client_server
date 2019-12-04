#include <stdio.h> // for perror
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <stdlib.h>

#include <vector>
#include <thread>
#include <mutex>

using namespace std;
vector<int> connected_list;
mutex m;

void echo(int childfd)
{
	while (true) {
		const static int BUFSIZE = 1024;
		char buf[BUFSIZE];

		ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
		if (received == 0 || received == -1) {
			perror("recv failed");
			break;
		}
		buf[received] = '\0';
		printf("%s\n", buf);

		ssize_t sent = send(childfd, buf, strlen(buf), 0);
		if (sent == 0) {
			perror("send failed");
			break;
		}
	}
}

void broad(int childfd)
{
	m.lock();
	connected_list.push_back(childfd);
	m.unlock();
	while(true)
	{
		const static int BUFSIZE = 1024;
		char buf[BUFSIZE];

		ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
		if (received == 0 || received == -1) {
			perror("recv failed");
			break;
		}
		buf[received] = '\0';
		printf("%s\n", buf);

		m.lock();
		for(vector<int>::iterator it = connected_list.begin(); it!=connected_list.end();it++)
		{
			ssize_t sent = send(*it, buf, strlen(buf), 0);
			if (sent == 0){
				perror("send failed");
				break;
			}
		}
		m.unlock();
	}
}

int main(int argc, char* argv[]) {
	if(argc !=2 && argc !=3)
	{
		printf("error!");
		return -1;
	}
	void (*broad_echo)(int);
	if(argc==3)									//-b인지 cmp해야됨
	{
		broad_echo = broad;
	}
	else
	{
		broad_echo = echo;
	}
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}

	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,  &optval , sizeof(int));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
	if (res == -1) {
		perror("bind failed");
		return -1;
	}

	res = listen(sockfd, 2);
	if (res == -1) {
		perror("listen failed");
		return -1;
	}

	while (true) {
		struct sockaddr_in addr;
		socklen_t clientlen = sizeof(sockaddr);
		int childfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr), &clientlen);
		if (childfd < 0) {
			perror("ERROR on accept");
			break;
		}
		printf("connected\n");
		thread th(broad_echo, childfd);
		th.detach();
	}

	close(sockfd);
}
