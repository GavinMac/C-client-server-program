//==============================================
//| C client-server program - By Gavin Macleod |
//|    Glasgow Caledonian University -2017     |
//|                 SERVER FILE		       |
//==============================================
// server.c - multi-threaded server using readn() and writen()

#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "rdwrn.h"

// thread function
void *client_handler(void *);

typedef struct utsname systemInfo;

struct timeval t1, t2;
int connfd = 0;
int listenfd = 0;
pthread_t sniffer_thread;

void send_ip(int);
void send_time(int);
void get_and_send_uname(int, systemInfo);
void send_msg(int);
void get_menu_choice(int, char*);
void send_file_list(int);
static void handler(int, siginfo_t*, void*);


//====================================================
//====== MAIN ========================================
//====================================================
// you shouldn't need to change main() in the server except the port number
int main(void)
{

    gettimeofday(&t1, NULL);

    struct sigaction act;

    memset(&act, '\0', sizeof(act));

    // this is a pointer to a function
    act.sa_sigaction = &handler;

    // the SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) == -1) {
	perror("sigaction");
	exit(EXIT_FAILURE);
    }

//----Socket Setup------------------------------------------------------
    int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50001);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
	perror("Failed to listen");
	exit(EXIT_FAILURE);
    }
//----End Socket Setup--------------------------------------------------

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");

	pthread_t sniffer_thread;
        // third parameter is a pointer to the thread function, fourth is its actual parameter
	if (pthread_create
	    (&sniffer_thread, NULL, client_handler,
	     (void *) &connfd) < 0) {
	    perror("could not create thread");
	    exit(EXIT_FAILURE);
	}
	//Now join the thread , so that we dont terminate before the thread
	//pthread_join( sniffer_thread , NULL);
	printf("Handler assigned\n");

    }

    // never reached...
    // ** should include a signal handler to clean up
    exit(EXIT_SUCCESS);
}
//====== END MAIN ====================================



//====================================================
//====== Client Handler ==============================
//====================================================
// thread function - one instance of each for each connected client
// Also includes menu
void *client_handler(void *socket_desc)
{
    //Get the socket descriptor
    int connfd = *(int *) socket_desc;

    systemInfo *uts;
    uts = (systemInfo *) malloc(sizeof(systemInfo));

    char *menu_choice = (char*) malloc(8);

    do { 

	printf("Waiting for client to select option...\n\n");
	send_msg(connfd);

	get_menu_choice(connfd, menu_choice);
	printf("Client %d choice was: %s\n", connfd, menu_choice);

	switch (*menu_choice) {
	case '1':
	    send_ip(connfd);
	    break;
	case '2':
	    send_time(connfd);
	    break;
	case '3':
	    get_and_send_uname(connfd, *uts);
	    break;
	case '4':
	    send_file_list(connfd);
	    break;
	case '5':
	    printf("File Transfer not implemented\n");
	    break;
	case '6': 
	    break;
	default:
	    printf("Invalid choice\n");
	}


    } while (*menu_choice != '6');


    if(menu_choice != NULL)
    {
	free(menu_choice);
    }


    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    // always clean up sockets gracefully
    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}  // end client_handler()


//====================================================
//====== Send Message ================================
//====================================================
//Send string message to the client
void send_msg(int socket)
{
    char msg_string[] = "\nPlease enter an option:";

    size_t n = strlen(msg_string) + 1;
    writen(socket, (unsigned char *) &n, sizeof(size_t));	
    writen(socket, (unsigned char *) msg_string, n);
 
} // end send_msg()



//====================================================
//====== Get Menu Choice =============================
//====================================================
//Gets the menu choice from the client
void get_menu_choice(int socketNumber, char *choice)
{
    size_t n;

    readn(socketNumber, (unsigned char *) &n, sizeof(size_t));	
    readn(socketNumber, (unsigned char *) choice, n);

    printf("Received: %zu bytes\n\n", n);

}// end get_menu_choice()


//====================================================
//====== Send IP and student ID as String ============
//====================================================
//Sends concatenated IP and Student ID to client
void send_ip(int socketNumber)
{
    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want an IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    char full_string[32];
    char student_id[] = "-S1715408";

    //Copy IP to full string
    strcpy(full_string, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    //Concatenate student ID to full string (IP)
    strcat(full_string, student_id);
 
    size_t n = strlen(full_string) + 1;

    writen(socketNumber, (unsigned char *) &n, sizeof(size_t));	
    writen(socketNumber, (unsigned char *) full_string, n);

    printf("Sent string: %s\n\n", full_string);

 
} // end send_ip()


//====================================================
//====== Send time as String =========================
//====================================================
//Sends server time as string to the client
void send_time(int socketNumber){

    time_t t;    // always look up the manual to see the error conditions
    //  here "man 2 time"
    if ((t = time(NULL)) == -1) {
	perror("time error");
	exit(EXIT_FAILURE);
    }

    // localtime() is in standard library so error conditions are
    //  here "man 3 localtime"
    struct tm *tm;
    if ((tm = localtime(&t)) == NULL) {
	perror("localtime error");
	exit(EXIT_FAILURE);
    }

    char *output = asctime(tm);

    size_t n = strlen(output) + 1;

    writen(socketNumber, (unsigned char *) &n, sizeof(size_t));	
    writen(socketNumber, (unsigned char *) output, n);

    printf("Sent time: %s\n", output);

}//end send_time()


//====================================================
//====== Get and Send Sysem info =====================
//====================================================
//Sends system information struct to the client using uname
void get_and_send_uname(int socketNumber, systemInfo uts)
{

    if (uname(&uts) == -1) {
	perror("uname error");
	exit(EXIT_FAILURE);
    }  

    size_t payload_length = sizeof(uts);

    size_t n = readn(socketNumber, (unsigned char *) &payload_length, sizeof(size_t));
    printf("payload_length is: %zu (%zu bytes)\n", payload_length, n);
    n = readn(socketNumber, (unsigned char *) &uts, payload_length);

    writen(socketNumber, (unsigned char *) &payload_length, sizeof(size_t));
    writen(socketNumber, (unsigned char *) &uts, payload_length);

    printf("Sent server info\n");

}  // end get_and_send_uname()


//====================================================
//====== Send File List ==============================
//====================================================
//Sends a file list as string to the client
void send_file_list(int socketNumber)
{

    DIR *mydir;
    if ((mydir = opendir("upload")) == NULL) {
	perror("error");
	exit(EXIT_FAILURE);
    }
    closedir(mydir);

    struct dirent *entry = NULL;

    size_t len = 0;

    //loop through entry to get size of all filenames as string.
    mydir = opendir("upload");
    while ((entry = readdir(mydir)) != NULL)
    {
	len = len + strlen(entry->d_name);
    }
    closedir(mydir);

    char filelist[len];

    //returns NULL when dir contents all processed
    mydir = opendir("upload");
    while ((entry = readdir(mydir)) != NULL)
    {
	strcat(strcat(filelist, entry->d_name),"\n");
    }
    closedir(mydir);

    strcat(filelist, "\0");

    size_t n = strlen(filelist);

    writen(socketNumber, (unsigned char *) &n, sizeof(size_t));
    writen(socketNumber, (unsigned char *) filelist, n);

    printf("Sent file list of size %zu bytes\n",n);

}//end send_file_list()

//Stat file
void stat_file(char *file)
{

    struct stat sb;

    if (stat(file, &sb) == -1) {
	perror("stat");
	exit(EXIT_FAILURE);
    }

}


//====================================================
//====== Signl Handler - SIGINT ======================
//====================================================
//Handles the SIGINT signal (Ctrl + C) displays server up-time
static void handler(int sig, siginfo_t *siginfo, void *context)
{
    printf("\nReceived a Ctrl+C (SIGINT)\n");
    printf("Shutting down...\n");

    pthread_join(sniffer_thread, NULL);

    shutdown(connfd, SHUT_RDWR);
    shutdown(listenfd, SHUT_RDWR);

    gettimeofday(&t2, NULL);

    double totalSeconds = (double) (t2.tv_usec - t1.tv_usec) / 1000000 + (double) (t2.tv_sec - t1.tv_sec) ;

    int seconds = ((int)totalSeconds % 60);
    int minutes = ((int)totalSeconds % 3600) / 60;
    int hours = ((int)totalSeconds % 86400) / 3600;
    int days = ((int)totalSeconds % (86400 * 30)) / 86400;

    printf ("\nServer up-time = %d days %d hours %d minutes and %d seconds\n", days, hours , minutes , seconds);

    //clean up sockets
    close(connfd);
    close(listenfd);

    exit(EXIT_SUCCESS);
}//end handler

