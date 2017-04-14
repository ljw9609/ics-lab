/*
 * proxy.c - CS:APP Web proxy
 *
 # Lv Jiawei 515030910036 ljw9609@sjtu.edu.cn
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"


typedef struct {
	int connfd;
	int port;
	struct sockaddr_in clientaddr;
}Client;
/*
 * Function prototypes
 */

void doit(int connfd,int port,struct sockaddr_in *clientaddr);
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void* thread(void *vargp);

/* reimplement rio functions */
ssize_t Rio_readn_w(int fd,void *usrbuf,size_t n);
void Rio_writen_w(int fd,void *usrbuf,size_t n);
ssize_t Rio_readlineb_w(rio_t *rp,void *usrbuf,size_t maxlen);
ssize_t Rio_readnb_w(rio_t *rp,void *usrbuf,size_t maxlen);

/* reimplement thread unsafe functions */
int open_clientfd_ts(char *hostname,int port);
int Open_clientfd_ts(char* hostname,int port);

int proxy_log;
sem_t mutex1;

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{

    int listenfd,connfd,port,clientlen;
    struct sockaddr_in clientaddr1;
    pthread_t tid;
    Client *client;

    proxy_log = open("proxy.log", O_RDWR|O_APPEND|O_CREAT,DEF_MODE);
    /* Check arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);
    
    signal(SIGPIPE, SIG_IGN);
    Sem_init(&mutex1, 0, 1);
    clientlen = sizeof(clientaddr1);
    listenfd = Open_listenfd(port);
    while(1){
    	client = (Client*)malloc(sizeof(client));
        connfd = Accept(listenfd,(SA *)&clientaddr1,&clientlen);
        client->port = port;
        client->connfd = connfd;
        client->clientaddr = clientaddr1;
        Pthread_create(&tid,NULL,thread,(void*)client);
        //doit(client); //do proxy
        //Close(connfd);
    }
    close(proxy_log);

    exit(0);
}

/*
 * doit - do proxy
 */
void doit(int connfd,int port,struct sockaddr_in *clientaddr){
    //struct stat sbuf;
    int serverfd; 
    char buf[MAXLINE];
    char method[MAXLINE],uri[MAXLINE],version[MAXLINE]; //request line
    char hostname[MAXLINE],pathname[MAXLINE];
    int server_port;
    int log_size = 0;
    char log_entry[MAXLINE];
    
    
    rio_t client_rio,server_rio;

    /* read request line */
    Rio_readinitb(&client_rio,connfd);
    ssize_t rc = Rio_readlineb_w(&client_rio,buf,MAXLINE);
    if(rc != 0){
    sscanf(buf, "%s %s %s",method,uri,version);
    if(strcasecmp(method,"GET")){
        printf("Not Implemented\n\n");
        return;
    }
    printf("request: %s\n",buf);
    
    /* parse uri from request line */
    int parse = parse_uri(uri,hostname,pathname,&server_port);
    printf("uri: %s\nhostname: %s\npathname: %s\nport: %d\n",uri,hostname,pathname,server_port);
    printf("parse: %d\n", parse);
    if(strstr(hostname,"firefox")){
        exit(0);
        //return;
    }
    if(strstr(hostname,"google")){
        exit(0);
        //return;
    }

    /* connect to server */
    serverfd = Open_clientfd_ts(hostname,server_port);
    if(serverfd < 0){
        printf("connection to server error.\n\n");
        return;
    }
    printf("connection to server succeed.\n\n");

    /* send request to server */
    Rio_readinitb(&server_rio,serverfd);

    
    char toserver[MAXLINE];
    sprintf(toserver,"%s %s %s\r\n",method,uri,version);
    printf("request line: %s",toserver);
    while(strcmp(buf,"\r\n")!=0){
        Rio_readlineb_w(&client_rio,buf,MAXLINE);
        printf("request: %s",buf);
        strcat(toserver,buf);
    }
    strcat(toserver,"\r\n");
    Rio_writen_w(serverfd,toserver,sizeof(toserver));
    
    char buf2[MAXLINE];
    ssize_t i;
    /*
    Rio_writen_w(serverfd,buf,strlen(buf));
        while(1){
            i = Rio_readlineb_w(&client_rio,buf2,MAXLINE);
            if(i == 2){
                printf("request:%s ",buf2);
                Rio_writen_w(serverfd,buf2,2);
                break;
            }
            else{
                printf("request:%s ",buf2);
                Rio_writen_w(serverfd,buf2,strlen(buf2));
            }  
        }
        /*
        if(i < 2){
            end = strlen(buf2);
            if(buf2[end-1] != '\n'){
                printf("error.\n");
                return;
            } 
        }*/
    

    //Rio_writen_w(serverfd,"\r\n",2);
    printf("client request done.\n");


    /* receive info from server and send to client */;    
    while((i = Rio_readnb_w(&server_rio,buf2,MAXLINE)) > 0){
        if(rio_writen(connfd,buf2,i) != i)
            //printf("response: %s",buf2);
            break;
        log_size += i;
    }

    printf("server response done.size: %d\n",log_size);

    /* write log file */
    format_log_entry(log_entry, clientaddr, uri, log_size);

    write(proxy_log,log_entry,strlen(log_entry));
    char blank[1];
    strcpy(blank," ");
    write(proxy_log,blank,strlen(blank));
    char size[MAXLINE];
    memset(size,0x00,sizeof(size));
    sprintf(size,"%d",log_size);
    write(proxy_log,size,strlen(size));
    char linebreak[1];
    strcpy(linebreak,"\n");
    write(proxy_log,linebreak,strlen(linebreak));
    

    printf("WriteLog done.\n\n");

    Close(serverfd);
    //Close(connfd);
    return;

    }
    else{
        printf("no request.\n");
        return;
    }
    //printf("finish.\n");
}

/*
 * reimplement rio functions
 */
ssize_t Rio_readn_w(int fd,void *usrbuf,size_t n){
    ssize_t i;
    if((i = rio_readn(fd,usrbuf,n)) < 0){
        printf("Rio_readn_w error.\n");
        return 0;
    }
    return n;
}

void Rio_writen_w(int fd,void *usrbuf,size_t n){
    if(rio_writen(fd,usrbuf,n) != n){
        printf("Rio_writen_w error.\n");
    }
}

ssize_t Rio_readnb_w(rio_t *rp,void *usrbuf,size_t maxlen){
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, maxlen)) < 0){
        printf("Rio_readnb_w error.\n");
        return 0;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp,void *usrbuf,size_t maxlen){
    ssize_t rc;

    if((rc = rio_readlineb(rp,usrbuf,maxlen)) < 0){
        printf("Rio_readlineb_w error.\n");
        return 0;
    }
    return rc;
}

/* concurrent function thread */
void* thread(void *vargp){
	Client *client = (Client*)vargp;
    int connfd = client->connfd;
    int port = client->port;
    struct sockaddr_in clientaddr = client->clientaddr;
	Free(vargp);
    Pthread_detach(pthread_self());
	doit(connfd,port,&clientaddr);
    Close(connfd);
	return NULL;
}

/* implement of thread unsafe functions */
int open_clientfd_ts(char *hostname,int port){
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;  /* check errno for cause of error */

    bzero((char * )&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    /* gethostbyname is thread unsafe,reimplement it */
    P(&mutex1);
    if((hp = gethostbyname(hostname)) == NULL)
        return -2;
    bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    V(&mutex1);

    serveraddr.sin_port = htons(port);
    if(connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;

}

/* wrapper of open_clientfd_ts */
int Open_clientfd_ts(char *hostname,int port){
    int rc;

    if ((rc = open_clientfd_ts(hostname, port)) < 0) {
    if (rc == -1)
        unix_error("Open_clientfd_ts Unix error");
    else        
        dns_error("Open_clientfd_ts DNS error");
    }
    return rc;
}

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	pathname[0] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


