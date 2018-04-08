//==============================================
//| C client-server program - By Gavin Macleod |
//|    Glasgow Caledonian University -2017     |
//|                 CLIENT FILE		       |
//==============================================
// client.c - message length headers with variable sized payloads
// also use of readn() and writen() implemented in separate code module

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <dirent.h>
#include "rdwrn.h"

//typedef struct utsname systemInfo;

//====================================================
//====== Get IP as String ============================
//====================================================
//Gets the IP and Student ID concatenated string from server
void get_ip(int socketNumber)
{
    char full_string[32];
    size_t k;

    readn(socketNumber, (unsigned char *) &k, sizeof(size_t));
    readn(socketNumber, (unsigned char *) &full_string, k);

    printf("\n---------------------\n");
    printf("Server IP-Student ID:\n%s",full_string);
    printf("\n---------------------\n");
    printf("Received: %zu bytes\n\n", k);

} // end get_ip()


//====================================================
//====== Get time as String ==========================
//====================================================
//Gets the server time from the server
void get_time(int socketNumber)
{
    char full_time[32];
    size_t k;

    readn(socketNumber, (unsigned char *) &k, sizeof(size_t));	
    readn(socketNumber, (unsigned char *) full_time, k);

    printf("Received: %zu bytes\n\n", k);
    printf("\n---------------------\n");
    printf("Server Time:\n%s", full_time);
    printf("\n---------------------\n");

} // end get_time()


//====================================================
//====== Get and Send Sysem info =====================
//====================================================
//Gets a system information struct from server using uname
void get_and_send_uname(int socket)
{
    struct utsname uts;

    if (uname(&uts) == -1) {
	perror("uname error");
	exit(EXIT_FAILURE);
    }

    size_t payload_length = sizeof(uts);

    // send the original struct
    writen(socket, (unsigned char *) &payload_length, sizeof(size_t));	
    writen(socket, (unsigned char *) &uts, payload_length);	 		

    // get back the altered struct
    readn(socket, (unsigned char *) &payload_length, sizeof(size_t));	   
    readn(socket, (unsigned char *) &uts, payload_length);

    // print out details of received & altered struct
    printf("\n---Server Info-------------------\n");
    printf("Node name:    %s\n", uts.nodename);
    printf("System name:  %s\n", uts.sysname);
    printf("Release:      %s\n", uts.release);
    printf("Version:      %s\n", uts.version);
    printf("Machine:      %s\n", uts.machine);
    printf("\n---------------------------------\n");

} // end send_and_get_uname()

//====================================================
//====== Get File List ===============================
//====================================================
//Gets a list of files from the server
void get_file_list(int socket)
{
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));
    char filelist[k];	
    readn(socket, (unsigned char *) filelist, k);

    printf("Received: %zu bytes\n\n", k);
    printf("\n---Files On Server -------------------\n");
    printf("%s", filelist);
    printf("\n--------------------------------------\n"); 

} // end get_file_list()



//====================================================
//====== Get msg Message ===========================
//====================================================
//Gets a message from the server
void get_msg(int socket)
{
    char msg_string[32];
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));	
    readn(socket, (unsigned char *) msg_string, k);

    printf("%s\n", msg_string);

} // end get_msg()


//====================================================
//====== Send Menu Choice ============================
//====================================================
//writes string to the server
void send_menu_choice(int socketNumber, char *choice)
{
    printf("Sending menu choice...\n");
    size_t n = strlen(choice) + 1;

    writen(socketNumber, (unsigned char *) &n, sizeof(size_t));
    writen(socketNumber, (unsigned char *) choice, n);

    printf("Sent choice: %s\n\n", choice);
}

//====================================================
//====== MAIN ========================================
//====================================================
int main(void)
{
    // *** this code down to the next "// ***" does not need to be changed except the port number
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Error - could not create socket");
	exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;

    // IP address and port of server we want to connect to
    serv_addr.sin_port = htons(50001);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // try to connect...
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	perror("Error - connect failed");
	exit(1);
    } else
       printf("Connected to server...\n");

    // ***
    // your own application code will go here and replace what is below... 
    // i.e. your menu etc.

//----Menu--------------------------------------------------------------

    char *choice = (char*) malloc(8);

    do {

	// get a string from the server
	get_msg(sockfd);

	printf("\nFile and Information System\n");
	printf("===========================\n");
	printf("1. Show IP and Student ID\n");
	printf("2. Display server time\n");
	printf("3. Display system information\n");
	printf("4. List files on server\n");
	printf("5. File Transfer\n");
	printf("6. Exit\n");
	printf("Enter choice: ");
	scanf("%s", choice);

	//Send input to server
	send_menu_choice(sockfd, choice);

	switch (*choice) {
	case '1':
	    get_ip(sockfd);
	    break;
	case '2':
	    get_time(sockfd);
	    break;
	case '3':
	    get_and_send_uname(sockfd);
	    break;
	case '4':
	    get_file_list(sockfd);
	    break;
	case '5':
	    printf("File transfer not yet implemented\n");
	    break;
	case '6':
	    printf("Exiting...\n");
	    break;
	default:
	    printf("Invalid choice\n");
	}

    } while (*choice != '6');

    if(choice != NULL)
    {
	free(choice);
    }


    // *** make sure sockets are cleaned up
    close(sockfd);
    exit(EXIT_SUCCESS);

} // end main()
