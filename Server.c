#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#define MAXUSERNAME 25
#define MAXUSERPASSWORD 25
#define MAXLINE MAXUSERNAME+MAXUSERPASSWORD
#define LENOFFOLLOWING 13
#define MAXFOLLOWING 25
#define PHPBUFFERSIZE 1024
#define LENOFDOTTXT 4
#define PORTNUM 5000
#define LISTENQUEUE 10


int Login(char* message)
{
	FILE* db= fopen("./names.txt","r");
	if(db==NULL)
	{
		perror("unable to open names");
		fclose(db);
		return -1;
	}
	char line[MAXLINE+1];
	while(fgets(line,MAXLINE+1,db)!=NULL)
	{
		if(strncmp(line,message,strlen(message)) == 0)
		{
			printf("match found");
			fclose(db);
			return 0;	
		}
	}
	fclose(db);
	return -1;
}
int Create(char* message)
{
	char* testname=malloc(strlen(message)+1);
	strcpy(testname,message);
	char* pwd= strchr(testname,' ')+1;
	*(strchr(testname,' '))='\0';
	if(strlen(testname)<6 || strlen(testname)>25)
	{
		free(testname);
		free(pwd);
		return -3;
	}
	if(strchr(testname,'<')!=NULL)//make sure there are no tags
	{
		free(testname);
		free(pwd);
		return -2;
	}
	if(strchr(pwd,' ')!=NULL)//make sure there are no spaces in name and pwd
	{
		free(testname);
		free(pwd);
		return -2;
	}
	free(testname);
	free(pwd);
	//simple validation done
	
	printf("creating account : %s\n",message);
	FILE* db= fopen("./names.txt","r");
	if(db==NULL)
	{
		perror("unable to open names");
		free(testname);
		free(pwd);
		fclose(db);
		return -4;
	}
	else
	{
		char line[MAXLINE+1];
		while(fgets(line,MAXLINE+1,db)!=NULL)
		{
			if(strncmp(line,message,(int) (line-strchr(line,' ') -1)) == 0)//compare tbe names to check if user exists already
			{
				printf("match found");
				free(testname);
				free(pwd);
				fclose(db);
				return -1;	
			}
		}
		fclose(db);
	}
	
	//name not in use
	
	db= fopen("./names.txt","a");
	if(db==NULL)
	{
		perror("unable to open names");
		fclose(db);
		return -4;
	}
	fprintf(db,"%s",message);
	printf("creating account : %s",message);
	fclose(db);
	
	char* uname=message;
	char* firstSpace= strchr(message,' ');
	*firstSpace='\0';
	strcat(uname,".txt");
	db= fopen(uname,"a");
	fclose(db);
	return 0;
	//created the tweet file for new user
}

int RdTwt(char* message, int connFd)
{
	printf("Reading tweets: %s is the message \n",  message);
	char filename[MAXUSERNAME+LENOFFOLLOWING];
	bzero(filename,MAXUSERNAME+LENOFFOLLOWING);
	//char* uname= message;
	strcat(filename,message);//start with the user name in the message sent
	strcat(filename,"FOLLOWING.txt");
	printf("opening file: %s\n", filename);
	FILE* followingFile= fopen(filename,"r");//openfile
	if(followingFile==NULL) perror("error encountered opening following file");
	char usersFollowed[MAXFOLLOWING][MAXUSERNAME] ;//potentially might raise from arbitrary limit
	int i=0;
	char * result;
	puts("Reading followers");
	for(;i<MAXFOLLOWING;i++)
	{
		result=fgets(usersFollowed[i],MAXUSERNAME,followingFile);
		if(result==NULL) break;
		else
		{
			puts(result);
			if(result[0]=='\0') i--;
		}
	}
	puts("Finished reading followers");
	fclose(followingFile);
	puts("closed FOLLOWING file");
	for(int j=0; j<i; j++)
	{
		char fileName[MAXUSERNAME+LENOFFOLLOWING];
		bzero(fileName,MAXUSERNAME+LENOFFOLLOWING);
		strcat(fileName,usersFollowed[j]);
		*(strchr(fileName,'\n'))='\0';
		strcat(fileName,".txt");
		FILE* tweetsFile=fopen(fileName,"r");
		printf("opening file: %s\n",fileName);
		if(tweetsFile==NULL) perror("Error opening file ");
		char writeBuffer[PHPBUFFERSIZE];
		bzero(writeBuffer,PHPBUFFERSIZE);
		puts("preparing to tranfer");
		result=fgets(writeBuffer,PHPBUFFERSIZE-1,tweetsFile);
		puts("transfering");
		if(result==NULL)
		{
			puts("this file has no data");
			fclose(tweetsFile);
			continue;
		}
		else
		{
			int x=strlen(result);
			puts("transfering");
			while(x!=0)
			{
				printf("%d bytes retrieved\n",x);
				write(connFd,writeBuffer,x);
				bzero(writeBuffer,PHPBUFFERSIZE);
				result=fgets(writeBuffer,PHPBUFFERSIZE,tweetsFile);
				if(result==NULL) break;
				else x=strlen(result);		
			}
			puts("transfer complete");
			fclose(tweetsFile);
		}
	}
	puts("function complete");
	return 0;
}
void WrTwt(char* message)
{
	printf("\n in WrTwt \n message recieved: %s\n",message);
	char* uname= message;
	char* text= strchr(uname,' ')+1;
	*(text-1) ='\0';
	char fileName[MAXUSERNAME+LENOFDOTTXT];
	strcat(fileName,uname);
	strcat(fileName,".txt");
	FILE* tweetsFile=fopen(fileName,"a");
	fprintf(tweetsFile,"%s", text);
	fclose(tweetsFile);
}
void Follw(char* message)
{
	char* uname= message;
	char* target=strchr(uname,' ')+1;
	*(target-1)='\0';
	printf("username is %s\n",uname);
	printf("target is %s\n",target);
	char fileName[MAXUSERNAME+LENOFFOLLOWING];
	strcat(fileName,uname);
	strcat(fileName,"FOLLOWING.txt");
	puts(fileName);
	FILE* tweetsFile=fopen(fileName,"a");
	if(tweetsFile==NULL)perror("problem opening");
	fprintf(tweetsFile,"%s", target);
	fclose(tweetsFile);
}
void UnFol(char* message)
{
	char * uname= message;
	char * untouch= strchr(uname,' ')+1;
	*(untouch-1)='\0';
	printf("username is %s \n unfollower is %s \n",uname,untouch);
	char filename[MAXUSERNAME+LENOFFOLLOWING];
	bzero(filename,MAXUSERNAME+LENOFFOLLOWING);
	strcat(filename,uname);	
	strcat(filename,"FOLLOWING.txt");
	printf("opening file: %s\n", filename);
	FILE* db= fopen(filename,"r+");
	if(db==NULL) perror("problem opening file ");
	char line[MAXUSERNAME];
	while(fgets(line,MAXUSERNAME,db)!=NULL)
	{
		puts("searching for user");
		if(strncmp(line,untouch,strlen(untouch))==0)
		{
			puts("user found");
			lseek(fileno(db),(strlen(line)*-1),SEEK_CUR);//file descriptor for db
			
			for(int i=0; i< strlen(untouch)-1;i++)
			{
				fprintf(db,"%c",'\0');
			}
			fprintf(db,"\n");
			fclose(db);
			return;
		}
	}
	fclose(db);
}
void delAc(char *message)
{
	char * uname= message;
	printf("username is %s \n",uname);
	FILE* db= fopen("names.txt","r+");
	if(db==NULL) perror("problem opening file ");
	char line[MAXUSERNAME];
	bzero(line,MAXUSERNAME);
	strcat(uname," ");
	while(fgets(line,MAXUSERNAME,db)!=NULL)
	{
		puts("searching for user");
		printf("uname = >%s< \n line = >%s<\n",uname,line);
		if(strncmp(uname,line,strlen(uname))==0)
		{
			puts("user found");
			lseek(fileno(db),(strlen(line)*-1),SEEK_CUR);//file descriptor for db
			
			for(int i=0; i< strlen(line)-1;i++)
			{
				fprintf(db,"%c",'\0');
			}
			fprintf(db,"\n");
			fclose(db);
			return;
		}
	}
	puts("user not found");
	fclose(db);
}
void processCommand(char* message,int connFd)
{
	//test to see which function is being called at the beginning
	if(strstr(message,"Login")==message)
	{
		message=strchr(message,' ')+1;//strip away method from call
		printf("logging in: %s\n",message);
		int success=Login(message);
		if(success==0)
		{
			printf("sending success \n");
			write(connFd,"proceed",sizeof("proceed"));
		}
		else write(connFd,"fail",sizeof("fail"));
	}
	else if(strstr(message,"RdTwt")==message)
	{
		message=strchr(message,' ')+1;
		printf("reading from: %s\n",message);
		RdTwt(message,connFd);
	}
	else if(strstr(message,"WrTwt")==message)
	{
		message=strchr(message,' ')+1;
		printf("writing in: %s\n",message);
		WrTwt(message);
	}
	else if(strstr(message,"Follw")==message)
	{
		message=strchr(message,' ')+1;
		printf("following in: %s\n",message);
		Follw(message);
	}
	else if(strstr(message,"UnFol")==message)
	{
		message=strchr(message,' ')+1;
		printf("Unfollowing in: %s\n",message);
		UnFol(message);

	}
	else if(strstr(message,"Create")==message)
	{
		puts("creating account");
		message=strchr(message,' ')+1;
		int success=Create(message);
		if(success==-1)
		{
			write(connFd,"username in use",sizeof("username in use"));
		}
		else if(success==-2)
		{
			write(connFd,"invalid characters used",sizeof("invalid characters used"));
		}
		else if(success==-3)
		{
			write(connFd,"invalid username length",sizeof("invalid username length"));
		}
		else if(success==-4)
		{
			write(connFd,"Serverside error: names.txt",sizeof("Serverside error: names.txt"));
		}
		else
		{
			write(connFd,"proceed",sizeof("proceed"));
		}
	}
	else if(strstr(message,"DelAc")==message)
	{
		puts("killing account");
		message=strchr(message,' ')+1;
		delAc(message);	
	}
}
void* createThread(void* args)
{
	printf("Created thread \n");
	int connfd= (intptr_t) args;
	char* buffer= malloc(PHPBUFFERSIZE);
	bzero(buffer,PHPBUFFERSIZE);
	printf("reading from fd#:%d \n",connfd);
	read(connfd,buffer,PHPBUFFERSIZE);
	printf("message recieved: %s\n",buffer);
	processCommand(buffer,connfd);
    close(connfd);
    free(buffer);
    pthread_exit(0);
}
int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '\0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORTNUM); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    listen(listenfd, LISTENQUEUE); 
	puts("starting server");
	pthread_t pid;
	while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        printf("Created fd#:%d\n",connfd);
        pthread_create(&pid, NULL, createThread, (void*)(intptr_t)connfd);
	}
}
