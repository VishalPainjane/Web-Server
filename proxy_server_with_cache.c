#include "proxy_parse.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<time.h>
#include<sys/wait.h>
#include<errno.h>
#include<pthread.h>
#include<semaphore.h>
#include<time.h>

#define MAX_CLIENTS 10
#define MAX_BYTES (1<<12)

typedef struct cache_element cache_element; // through this, it will replace (struct cache_element) -> (cache_element)

struct cache_element{
    char* data;
    int len;
    char* url;
    time_t lru_time_track; // we will be inplementing lru cache throug time based, such element that is not updated a long time, it will be removed
    cache_element* next;  // struct cache_element* next; ...due to typedef
    /*
    this is a linked list that will store elements(mainly the urls and its info into a block)

    */
};

cache_element* find(char* url);
int add_cache_element(char* data, int size, char* url);
void removed_cache_element();

/*
when ever multiple people will send request to the WEBSERVER, 
it will be working in multithreaded env

proxy server will have different sockets, so when ever someone 
is requesting something from the server, it will communicate via sockets
*/
int port_number = 8080;
int proxy_socketId;
pthread_t tid[MAX_CLIENTS];
sem_t semaphore;
pthread_mutex_t lock;
/*
when ever using shared resources(multiple sockets/ports) to LRU cache, 
there may occure RACE CONDITION

this can he handeled via LOCKS(the port that is accessing the proxy)(0/1), 
and SEMAPHORE(multi valued) is number if users at start(i.e 10) which will decrease until the value is 0
then all the request that arives after it will be in WAITING STATE
*/

cache_element *head;
int cache_size;


int connectRemoteServer(char* host_addr, int port_num){
    int remotesocket = socket(AF_INET, SOCK_STREAM, 0);
    if(remotesocket<0){
        printf("Error in creating your socket\n");
    }
    struct hostent* host = gethostbyname(host_addr);
    if(host==NULL){
        fprintf(strerr, "No such host exist\n");
        return -1;
    }
    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);

    bcopy((char *)&host->h_addr, (char *)&server_addr.sin_addr, host->h_lenght);
    if(connect(remotesocket, (struct sockaddr *)&server_addr, (size_t)sizeof(server_addr)<0)){
        fprintf(stderr, "Error in connecting\n");
        return -1;
    }

    return remotesocket;
}

int handle_request(int client_socketId, ParsedRequest *req, char* tempreq){
    char *buf = (char *)malloc(sizeof(char)*MAX_BYTES);
    strcpy(buf, "GET");
    strcat(buf, req->path);
    strcat(buf, " ");
    strcat(buf, req->version);
    strcat(buf, "\r\n");

    size_t len = strlen(buf);

    if(ParsedHeader_set(req, "Connection", "close")<0){
        printf("set header key is not working");
    }

    if(ParsedHeader_get(req, "Host") == NULL){
        if(ParsedHeader_set(req, "Host", req->version)<0){
            printf("set host header key is not working");
        }
    }

    if(ParsedRequest_unparse_headers(req, buf+len, (size_t)MAX_BYTES-len)<0){
        printf("unparse Failed");
    }

    int server_port = 80;
    if(req->port!=NULL){
        server_port = atoi(req->port);
    }

    int remotesockteid = connectRemoteServer(req->host, server_port);

}

void *thread_function(void *socketnew){
    sem_wait(&semaphore);
    int p;
    sem_getval(&semaphore, p);
    printf("semaphore val is; %d\n", p);
    int *t = (int*) socketnew;
    int socket = *t;
    int bytes_send_client , len;

    char *buffer = (char*)calloc(MAX_BYTES, sizeof(char));
    bzero(buffer, MAX_BYTES);
    bytes_send_client = recive(socket, buffer, MAX_BYTES, 0);

    while(bytes_send_client > 0){
        len = strlen(buffer);
        if(strstr(buffer, "\r\n\r\n") == NULL){
            bytes_send_client = recv(socket, buffer+len, MAX_BYTES-len, 0);
        }else{
        break;
        }
    }

    char * tempreq = (char *)malloc(strlen(buffer)*strlen(sizeof(char)+1));

    for(int i = 1; i<strlen(buffer);i++){
        tempreq[i] = buffer[i];
    }

    struct cache_element* temp = find(tempreq);
    if(temp!=NULL){
        int size = temp->len/sizeof(char);
        int pos = 0;
        char response[MAX_BYTES];
        while(pos<size){
            bzero(response, MAX_BYTES);
            for(int i =0;i<MAX_BYTES;i++){
                response[i] = temp->data[i];
                pos++;
            }
            send(socket, response, MAX_BYTES, 0);
        }

        printf("data retrived from cache\n");
        printf("%s\n\n", response);

    }
    else if(bytes_send_client){
        len = strlen(buffer);
        ParsedRequest *req = ParsedRequest_parse();

        if(ParsedRequest_parse(req, buffer,len) < 0){
            printf("Parsing Falied\n");
        }
        else{
            bzero(buffer, MAX_BYTES);
            if(!strcmp(reqest->method, "GET")){
                if(req->host && req->path & checkHTTPversion(req->version) == 1){
                    bytes_send_client = handle_request(socket, req, tempreq);
                    if(bytes_send_client == -1){
                        sendErrorMessage(socket, 500);
                    }
                }
                else{
                    sendErrorMessage(socket, 500);
                }
            }
            else{
                printf("this code doesn't support any method apart from GET\n");
            }
        }

        ParsedRequest_destroy(req);
    }else if(bytes_send_client == 0){
        printf("client disconnected");

    }
    shutdown(socket, SHUT_RDWR);
    close(socket);
    free(buffer);
    sem_post(&semaphore);
    sem_getval(&semaphore, p);
    printf("semaphore post values is %d", p);
    free(tempreq);
    return NULL;
}

int main (int argc, char* argv[]){
    // printf("%d\n", argc);
    // for(int i=0;i<argc;++i){
    //     printf("%s\n", argv[i]);
    // }

    int client_socketId, client_len;
    struct sockaddr_in server_addr, client_addr; // pre-defined structure for address
    
    sem_init(&semaphore, 0,MAX_CLIENTS); // setup semaphore
    pthread_mutex_init(&lock, NULL); // setup mutex, set as default NULL, else it will be set as garbage value

    if (argc==2){
    port_number= atoi(argv[1]); // atoi - this will parse the cmd and provide it as str
    // example==> ./proxy 8090
    }else{
        printf("Too few Arguments\n");
        exit(1); // system calls, this will exit the server
    }

    printf("Starging proxy server at port: %d\n", port_number);


    /* 
    proxy will have only 1 socket,to which evey client will request
    as soon as the request gets accepted, it will spawn a new thread(with new threadid)
    through that new spawned socket, all the info will be transmited
    */
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0); //this uses IPv4, and happend accrose TCP connection(secured connection, hand shake)

    if (proxy_socketId==0){
        perror("Failed to create socket")
    }

    int reuse = 1;
    if(setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse))<0){
        perror('setSockOpt Failed\n');
    }

    bzero((char*)&server_addr, sizeof(server_addr)); // where ever we create and init struck in C, it will hold garbage value, so we have to clear(init it with 0)

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number); //htons in used for the machine understand the port number in simple terms
    server_addr.sin_addr.s_addr = INADDR_ANY; // in this scoket, this server address which is going to communicate to another server, address of another server will be set to anything

    if(bind(proxy_scoketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Port is not available\n");
        exit(1);
    }

    printf("Binding on port %d\n", port_number);
    
    int listen_status = listen(proxy_socketId, MAX_CLIENTS);

    if(listen_status<0){
        printf("Error in listening\n");
        exit(1);
    }

    int i = 0;
    int Connect_socketId[MAX_CLIENTS];

    while(1){
        bzero((char*) &client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketId = accept(proxy_scoketId, (struct *sockaddr)&client_addr, (socklen_t*)&client_len);
        
        if(listen_status<0){
            printf("Not able to connect socket\n");
            exit(1);
        }else{
            Connect_socketId[i] = client_socketId;
        }

        struct sockaddr_in * client_pt = (struct sockaddr_in*)&client_addr;
        struct in_addr ip_addr = client->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);

        printf("Client is connected with port number %d and ip address %s\n", ntohs(client_addr.sin_port), str);

        pthread_create(&tid[i], NULL, thread_function, (void *)&Connect_socketId[i]);
        i++;

        }

        close(proxy_socketId);
        return 0;

}