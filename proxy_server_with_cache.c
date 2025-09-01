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

int main (int argc, char* argv[]){
    int client_socketId, client_len;
    struct sockaddr server_addr, client_addr; // pre-defined structure for address
    
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
        perror("Failed to create a ")
    }

}