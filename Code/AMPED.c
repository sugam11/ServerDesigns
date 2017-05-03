#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#define HTTPRequestSize 1024


void sig_handler(int signo){
    printf("SIGPIPE Encountered\n");
}

char *find_content_type (char *file_ext) {
    char *p;  // pointer to the type found
    char buf2[64];
    
    p = (char *)malloc(64);
    
    /* find the type: */
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
    else if ( strcmp(file_ext, "avi") == 0) {
        strcpy (buf2, "Content-Type: video/avi \r\n");
    }
    else if ( strcmp(file_ext, "c") == 0) {
        strcpy (buf2, "Content-Type: text/x-c \r\n");
    }

    else {
        strcpy (buf2, "Content-Type: image/ico \r\n");
    }
    
    p = buf2;
    // printf ("content-type: %s\n", p);
    //return "Content-type: image/jpeg\r\n";
    return p;
}


int main(int argc,char **argv){
    
    if(argc < 2){
        printf("./a.out <portnum>\n");
        exit(0);
    }
    
    
    //FDs declarations
    int listenfd,connfd,sockfd,maxfd;
    fd_set rset,allset;
    
    int nready,max_index = -1,recv_size;
    int client[FD_SETSIZE];
    char buf[HTTPRequestSize];              //Receive HTTP request Buffer
    
    
    //Client defs
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    
    //Server defs
    
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    
    bzero (&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    servaddr.sin_port = htons (atoi(argv[1]));
    
    if(bind(listenfd,(struct sockaddr *)& servaddr,sizeof(servaddr)) > 0){
        printf("Server bound at port num : %s\n",argv[1]);
    }
    
    printf("Enter max number of connections \n");
    int max_connections = 0;
    scanf("%d",&max_connections);
    
    if(listen(listenfd,max_connections) > 0){
        printf("Server listening at port num : %s\n",argv[1]);
    }
    
    maxfd = listenfd;
    for (int i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;		/* -1 indicates available entry */
    
    FD_ZERO(&allset);
    FD_SET(listenfd,&allset);
    
    //Buffers to process HTTP Request
    char buffer_copy[HTTPRequestSize];
    char del[3] = "\r\n";
    char file_del[2] = " ";
    char file_ext_del[3] = " .?";
    char header_line_copy[256],file_name_copy[128],*temp;
    char *header_line,*file_name,*file_ext;
    
    
    
    //SIGPIPE Handler
    signal(SIGPIPE,sig_handler);
    
    while(1){
        rset = allset;
        nready = select(maxfd + 1,&rset,NULL,NULL,NULL);
        if(FD_ISSET(listenfd,&rset)){       /* new client connection */
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
            printf ("new client ip: %s, port %d\n",inet_ntoa(cliaddr.sin_addr),ntohs (cliaddr.sin_port));
            
            int clients;
            for(clients = 0;clients < FD_SETSIZE;clients++){
                if(client[clients] < 0){
                    client[clients] = connfd;     //Save client descriptor
                    break;
                }
            }
            if(clients == FD_SETSIZE){
                printf ("too many clients");
                exit(0);
            }
            
            FD_SET (connfd, &allset);	/* add new descriptor to set */
            if (connfd > maxfd)
                maxfd = connfd;	/* for select */
            
            if(clients > max_index){
                max_index = clients;          //Max index in client array
            }
            
            if (--nready <= 0)
                continue;		/* no more readable descriptors */
        }
        
        for (int i = 0; i <= max_index; i++) {      //Checking all clients for data
            if((sockfd = client[i]) < 0){
                continue;
            }
            if(FD_ISSET(sockfd,&rset)){
                if((recv_size = read(sockfd,buf,HTTPRequestSize)) == 0){
                    //Client closed connection
                    printf("Closed connection with fd : %d\n",sockfd);
                    close(sockfd);
                    FD_CLR (sockfd, &allset);
                    client[i] = -1;
                }
                else{
                    //Read HTTP Request
                    printf("%s\n",buf);
                    
                    memset(buffer_copy,'\0',sizeof(buffer_copy));
                    strcpy(buffer_copy,buf);
                    
                    header_line = strtok(buffer_copy,del);
                    temp = strtok(NULL,del);
                    temp = strtok(NULL,del);
                    
                    memset(header_line_copy,0,sizeof(header_line_copy));
                    strcpy(header_line_copy,header_line);
                    
                    file_name = strtok(header_line_copy,file_del);
                    file_name = strtok(NULL,file_del);
                    memmove(file_name, file_name+1, strlen(file_name));
                    
                    memset(file_name_copy,0,sizeof(file_name_copy));
                    strcpy(file_name_copy,file_name);
                    
                    file_ext = strtok(file_name_copy,file_ext_del);
                    file_ext = strtok(NULL,file_ext_del);
                    
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
                        printf("Sending 404 error\n");
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
                        if(write(sockfd,response,strlen(response)) < strlen(response)){
                            perror("Error sending");
                        }                
                    }
                    
                    else{
                        printf("Sending File : %s\n",file_name);
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
                        if(fork() == 0){
                            printf("New process created with id : %d\n Serving client : %d\n\n\n",getpid(),sockfd);
                            write(sockfd,header_buff,strlen(header_buff));
                            while((n = read(file_des,send_buf,1024)) > 0){
                                write(sockfd,send_buf,n);
                            }
                            printf("File transer Complete\n");
                            close(file_des);
                            printf("Process exited with id : %d\n",getpid());
                        }
                        //Handle zombies
                        int status;
                        while (waitpid(-1, &status, WNOHANG) > 0);
                        
                    }
                }
                if (--nready <= 0)
                    break;		/* no more readable descriptors */
            }
        }
    }
    
}
