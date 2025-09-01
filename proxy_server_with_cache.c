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

struct cache_element(
    char* data;
    int len;
    char* url;
    time_t lru_time_track; // we will be inplementing lru cache throug time based, such element that is not updated a long time, it will be removed
    cache_element* next;  // struct cache_element* next; ...due to typedef
    /*
    this is a linked list that will store elements(mainly the urls and its info into a block)

    */
);

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
int proxy_socket_id;
pthread_t tid[MAX_CLIENTS]
sem_t semaphore;
pthread_mutex_t lock;
/*
when ever using shared resources(multiple sockets/ports) to LRU cache, 
there may occure RACE CONDITION

this can he handeled via LOCKS(the port that is accessing the proxy)(0/1), 
and SEMAPHORE(multi valued) is number if users at start(i.e 10) which will decrease until the value is 0
then all the request that arives after it will be in WAITING STATE
*/

cache_elemtnt* head;
int cache_size;

int main (int argc, char* argv[]){
    int client_socketId, client_len;
    struct sockaddr server_addr, client_addr;
    sem_init(&semaphore, MAX_CLIENTS);
    
}