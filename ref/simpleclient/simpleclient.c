#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>

#define SERVER_IP    "192.168.19.46"
#define SERVER_PORT  6379

int fd;
int redisConnect(const char * ip, int port){
	fd = socket(AF_INET,SOCK_STREAM,0);
    if( fd == -1 ) {
        perror("redisConnect -> Create fd error");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; // ipv4
    server_addr.sin_port   = htons(port);
    inet_pton(AF_INET,ip,&server_addr.sin_addr.s_addr);

    int ret = connect(fd,(struct sockaddr*)(&server_addr),
                      sizeof(server_addr));
    if( ret == -1 ) {
        perror("redisConnect -> Connect error !");
		return -1;
    }
    
    return 0;
}

void * redisCommand(int fd,const char *cmd){

}

volatile uint8_t working = 0; 

void*redisRecvDaemon(void * arg){
    char recv_buf[1024];
    int len;
	int rv ;
	fd_set read_set;
	struct timeval timeout;
    while(working ) {
		FD_ZERO(&read_set);
		FD_SET(fd,&read_set);
	    timeout.tv_sec = 5;
	    timeout.tv_usec= 0;
		rv = select(fd+1,&read_set,NULL,NULL,&timeout);
		if( rv <= 0 ) continue;
        // recv and print 
        memset(recv_buf,0,sizeof(recv_buf));
        len = recv(fd,recv_buf,sizeof(recv_buf),0);
        recv_buf[len] = '\0';
        printf("<- %s",recv_buf);
		fflush(stdout);
    }
}

int startWith(const char * origin, const char * prefix) {
	if( origin == NULL || prefix == NULL || 
        strlen(origin) < strlen(prefix))    return 0;
	
	for( int i=0;i<strlen(prefix) ;i++ ) {
		if( origin[i] != prefix[i]) return 0;
	}
	return 1;
}

int countWord(const char* cmd) {
	int32_t count = 0;
	uint8_t word_flag = 0;
	for( int i=0;i<strlen(cmd); i++ ) {
		if( cmd[i] != ' ' && word_flag == 0 ) {
			word_flag = 1;
		}
		
		if( cmd[i] == ' '  && word_flag == 1 ) {
			word_flag = 0;
			count ++;
		}
	}
	return count+1;// the last one word is missing
}

int getOneWord(char * cmd,int * offset) {
	int count = 0;
	int word_flag = 0;
	(*offset ) = 0;
	for( int i=0;i<strlen(cmd)+1;i++ ){
		if( cmd[i] == ' ' && word_flag == 0 ){
			(*offset) ++;
		}else if( cmd[i] != ' '&& word_flag == 0) {
			word_flag = 1;
			count ++;
		}else if( (cmd[i] != ' ')&& (cmd[i]!= '\0')
                  && word_flag == 1) {
			count ++;
		}else if((cmd[i] == ' ' || cmd[i] == 0)
                  && word_flag == 1) {
			word_flag = 0;
			return count;
		}else{}
	}
	return 0;
}

int simpleParser(char* raw_cmd, char*cmd){
	if( strcmp(raw_cmd,"PING" )== 0
        ||strcmp(raw_cmd,"ping" )== 0 ) {
		strcpy(cmd,"PING\r\n");
		return 0;
	}
    if( strlen( raw_cmd ) >= 5 ) {  // "set x" or "get x" must longer than 5
    	// simple set/get cmd
		int count = 0;
		int offset = 0;
		int word_len = 0;
       	if( startWith(raw_cmd,"set ")|| startWith(raw_cmd,"get ")) {
	        count = countWord(raw_cmd);
			sprintf(cmd,"*%d\r\n",count);
			cmd = cmd + strlen(cmd);
			for( int j=0;j<count;j++ ) {
				word_len = getOneWord(raw_cmd,&offset);
				raw_cmd = raw_cmd + offset;
				raw_cmd[word_len] = '\0';
				sprintf(cmd,"$%d\r\n%s\r\n",word_len,raw_cmd);
				raw_cmd = raw_cmd + word_len + 1;
				cmd = cmd + strlen(cmd);
			}
			return 0;
		}
	}
	
	return -1;
}
int main(){
    int err = redisConnect(SERVER_IP,SERVER_PORT);
    if( err != 0 ) {
        return 0;
    }
    working = 1;
    pthread_t redis_recv_deamon;
    err = pthread_create(&redis_recv_deamon,NULL,redisRecvDaemon,NULL);
	if( err != 0 ) {
		perror("main -> Create thread error !");
		return 0;
	}
    char  raw_cmd_buf[1024];
	char  cmd_buf[1024];
    int   cmd_len ;
    while( 1 ) {
        // scanf("%s",raw_cmd_buf);
		fgets(raw_cmd_buf,sizeof(raw_cmd_buf),stdin);
		// remove newline flag \n
		if( raw_cmd_buf[strlen(raw_cmd_buf)-2] == '\r') {
			raw_cmd_buf[strlen(raw_cmd_buf)-2] = '\0';
		}
		if( raw_cmd_buf[strlen(raw_cmd_buf)-1] == '\n') {
			raw_cmd_buf[strlen(raw_cmd_buf)-1] = '\0';
		}
        if( strcmp(raw_cmd_buf,"exit") == 0) break;
		// append \r\n
	    // cmd_len = strlen(cmd_buf);
		// cmd_buf[cmd_len] = '\r';
		// cmd_buf[cmd_len+1] = '\n';
        // cmd_buf[cmd_len+2] = '\0';	
		err = simpleParser(raw_cmd_buf,cmd_buf);
		if( err != 0 ) {
			printf("note: command is invalid! \r\n");
			continue;
		}
        send(fd,cmd_buf,strlen(cmd_buf),0);
        printf("-> %s",cmd_buf);
        fflush(stdout);
    } 

    working = 0;
    pthread_join(redis_recv_deamon,NULL);
    return 0;
}


