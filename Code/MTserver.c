#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

static void *doit(void *);      /* each thread executes this function */

char *find_content_type (char *file_ext) {
    char *p;  // pointer to the type found
    char buf2[64];
    
    p = (char *)malloc(64);
    
    if ( strcmp(file_ext, "html") == 0 || strcmp (file_ext, "hml") == 0) {
        strcpy (buf2, "Content-Type: text/html \r\n");
    }
    
    else if ( strcmp(file_ext, "txt") == 0) {
        strcpy (buf2, "Content-Type: text/plain \r\n");
    }
    else if ( strcmp(file_ext, "pdf") == 0) {
        strcpy (buf2, "Content-Type: application/pdf \r\n");
    }
    else if ( strcmp(file_ext, "doc") == 0) {
        strcpy (buf2, "Content-Type: application/msword \r\n");
    }
    else if ( strcmp(file_ext, "jpg") == 0 || strcmp (file_ext, "jpeg") == 0) {
        strcpy (buf2, "Content-Type: image/jpeg \r\n");
    }
    
    else if ( strcmp(file_ext, "gif") == 0) {
        strcpy (buf2, "Content-Type: image/gif \r\n");
    }
    
    else if ( strcmp(file_ext, "ico") == 0) {
        strcpy (buf2, "Content-Type: image/gif \r\n");
    }
    
    else {
        strcpy (buf2, "Content-Type: image/ico \r\n");
    }
    
    p = buf2;
    return p;
}

void childFxn(int accept_fd){
    int i,recvRqstSize;
    
    unsigned int clilen = sizeof(struct sockaddr_in);
    char buffer[1024],buffer_copy[1024];
    char del[3] = "\r\n";
    char file_del[2] = " ";
    char file_ext_del[3] = " .?";
    char header_line_copy[256],file_name_copy[128],*temp;
    char *header_line,*file_name,*file_ext;
    struct pollfd mypoll = { accept_fd, POLLIN|POLLPRI };
    
    sigignore(SIGPIPE);
            while (1) {
                if( poll(&mypoll, 1, 300000) )
                    recvRqstSize = recv(accept_fd,buffer,sizeof(buffer),0);
                else{
                    close(accept_fd);
                    break;
                }                
                printf("%s\n",buffer);
                fflush(stdout);
                memset(buffer_copy,'\0',sizeof(buffer_copy));
                strcpy(buffer_copy,buffer);
                
                //memset(header_line,0,sizeof(header_line));
                header_line = strtok(buffer_copy,del);
                temp = strtok(NULL,del);
                temp = strtok(NULL,del);
               // printf("%s\n",header_line);
                
                memset(header_line_copy,'\0',sizeof(header_line_copy));
                strcpy(header_line_copy,header_line);
                
                //printf("asfasf : %s\n",header_line_copy);
                
                //memset(file_name,0,sizeof(file_name));
                file_name = strtok(header_line_copy,file_del);
                file_name = strtok(NULL,file_del);
                memmove(file_name, file_name+1, strlen(file_name));
               // printf("File name is : %s\n",file_name);
                
                memset(file_name_copy,'\0',sizeof(file_name_copy));
                strcpy(file_name_copy,file_name);
                
                file_ext = strtok(file_name_copy,file_ext_del);
                file_ext = strtok(NULL,file_ext_del);
               // printf("File ext is : %s\n",file_ext);
                
            if(strcmp(file_ext,"cgi") != 0){
                int file_des;
                printf("file name is : %s\n ",file_name);
                file_des = open(file_name,O_RDONLY,0);
                struct stat st;
                fstat(file_des,&st);
                int file_size = st.st_size;
                
                char status_code[50],response[1024];
                char header_buff [512];
                if(file_des == -1){
                    //404 NOT FOUND
                    strcpy(status_code,"404 Not Found\r\n");
                    strcpy(response,"HTTP/1.1 ");
                    strcat(response,status_code);
                    
                    time_t clk = time(NULL);
                    strcat(response,"Date: ");
                    strcat(response,ctime(&clk));
                    //strcat(response,"\r\n");
                    
                    strcat(response,"Server: ");
                    strcat(response,"webserver/0.1 (Mac64)\r\n");
                    
                    strcat(response,"Content-Length: 230\r\n");
                    
                    strcat(response,"Connection : Closed\r\n");
                    strcat(response,"Content-Type: text/html; charset=iso-8859-1\r\n");
                    
                    strcat(response,"\r\n");
                    
                    strcat(response,"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>");
                    if(send(accept_fd,response,strlen(response),0) < strlen(response)){
                        perror("Error sending");
                    }                
                }
                else{
                    char send_buf[1024];
                    int n;
                    strcpy (header_buff, "HTTP/1.1 200 OK\r\nContent-Length: ");
                    /* content-length: */
                    char str_file_size[10];
                    sprintf(str_file_size,"%d",file_size);
                    strcat (header_buff, str_file_size);
                    strcat (header_buff, "\r\n");
                    /* content-type: */
                    strcat (header_buff, find_content_type (file_ext));
                    //printf ("%s\n", find_content_type (file_ext));
                    strcat (header_buff, "Connection: keep-alive\r\n\r\n");
                    send(accept_fd,header_buff,strlen(header_buff),0);
                    while((n = read(file_des,send_buf,1024)) > 0){
                        send(accept_fd,send_buf,n,0);
                    }
                    close(file_des);
                }
            
        }           
                
        }
    close(accept_fd);
    exit(0);
}

static void *doit(void *arg)
{
    pthread_detach(pthread_self());
    childFxn((int) arg);  /* same function as before */
    close((int) arg);
    return(NULL);
}

int main(int argc, char **argv)
{
    int             listenfd, connfd;
    socklen_t       addrlen, len;
    pthread_t tid1;
    struct sockaddr *cliaddr;
    struct sockaddr_in servaddr;
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(atoi(argv[1]));
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    listen(listenfd,10);

    cliaddr = malloc(addrlen);

    for ( ; ; ) {
        len = addrlen;
        connfd = accept(listenfd, cliaddr, &len);
        pthread_create(&tid1, NULL, &doit, (void *) connfd);
    }
}
