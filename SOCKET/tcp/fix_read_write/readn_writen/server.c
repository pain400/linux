#include "tcp_socket.h"
#define PORT 8000

void handle_err(char *);
void handle_sig(int);

int main(){
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	int addrlen;
	int listenfd;
	
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(listenfd==-1){
		handle_err("socket error\n");
	}
	
	bzero(&serveraddr,sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);

	int optval = 1;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
	if(bind(listenfd,(struct sockaddr *)&serveraddr,sizeof(struct sockaddr_in))==-1){
		close(listenfd);
		handle_err("bind error\n");
	}
	
	if(listen(listenfd,128)==-1){
		close(listenfd);
		handle_err("listen error\n");
	}
	
	addrlen = sizeof(struct sockaddr_in);
	int connfd = accept(listenfd,(struct sockaddr *)&serveraddr,&addrlen);
	if(connfd==-1){
		close(listenfd);
		handle_err("accept error\n");
	}
	
	pid_t pid = fork();
	if(pid==0){
		signal(SIGUSR1,handle_sig);
		struct packet send_buf;
		int n;
		while(fgets(send_buf.buf,sizeof(send_buf.buf),stdin)!=NULL){
			n = strlen(send_buf.buf);
			send_buf.len = htonl(n);
			writen(connfd,&send_buf,n+4);
			memset(&send_buf,0,sizeof(send_buf));
		}
	}else if(pid>0){
		struct packet recv_buf;
		int n;
		memset(&recv_buf,0,sizeof(struct packet));
		while(1){
			int len = readn(connfd,&recv_buf.len,4);
			if(len==-1){
				break;
			}else if(len<4){
				printf("client closed\n");
				break;
			}
			n = ntohl(recv_buf.len);	
			len = readn(connfd,recv_buf.buf,n);
			if(len==-1){
				break;
			}else if(len<n){
				printf("client closed\n");
				break;
			}else{
				fputs(recv_buf.buf,stdout);
				memset(&recv_buf,0,sizeof(struct packet));
			}
		}
		close(connfd);
		kill(pid,SIGUSR1);
		handle_err("readn error\n");
	}else{
		handle_err("fork error\n");
	}
}

void handle_err(char *err){
	perror(err);
	exit(-1);
}

void handle_sig(int num){
	printf("receive sig %d\n",num);
	exit(0);
}
