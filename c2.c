/*
    FILE:   c2.c
    RUN:    gcc c2.c -o c2
            ./c2
    NAME:   Dhruv Rawat
    ID:     2019B3A70537P
    SEC:    P2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <errno.h>

#define BUFLEN 10000            // Max length of buffer
#define PORT 8537               // The port on which to send data
#define RETRANS_TIMEOUT_SEC 2   // Retransmission timeout in seconds

#define RED   		"\033[0;31m"
#define GREEN 		"\033[0;32m"
#define YELLOW 		"\033[0;33m"
#define BLUE 		"\033[0;34m"
#define PURPLE 		"\033[0;35m"
#define CYAN 		"\033[0;36m"
#define INVERT		"\033[0;7m"
#define RESET  		"\e[0m" 
#define BOLD		"\e[1m"
#define ITALICS		"\e[3m"
#define UNDERLINE	"\e[4m"

typedef struct packet {
    int payload_id;
    int seq_no;
    char data[BUFLEN];
    int payload_size;
    int TYPE;       // 0 for DATA, 1 for ACK
    bool isLast;    // To check if sent data packet is last packet
} PKT;

void printError(char *s) {
    perror(s);
    exit(1);
}

int main(void)
{
    struct sockaddr_in server_addr;
    int sock_fd;
    char buf[BUFLEN];
    PKT send_pkt, rcv_ack;

    //Socket creation
    if ((sock_fd=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printError("socket");
    }    

    //Initializing socket
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


    //Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printError("Error connecting to server");
    }

    //Open file to read data from
    FILE *input = fopen("id.txt", "r");

    if (input == NULL) {
        printError("Error opening file");
        exit(EXIT_FAILURE);
    }        

    char *token;
    fgets(buf, BUFLEN, input);      // Read the line from the file which contains comma-separated IDs. 
    // Assumption: The file only contains a single line which has IDs separated by a comma and terminated by a period

    int state = 0;
    int start = 1;
    int seq = 0;

    while (1) {
        switch(state) { 
            case 0: {
                    //Tokenizing the buffer on the basis of commas and periods
                    if(start) {
                        token = strtok(buf, ",.");
                        start = 0;
                    } else {
                        token = strtok(NULL, ",.");
                    }
                    if(token!=NULL) {
                        send_pkt.isLast = false;
                    } else {
                        // If token is NULL, it means the buf is completely tokenized
                        // Send an empty dummy packet to tell server that the file has ended
                        token = "";
                        send_pkt.isLast = true;
                    }
                    strcpy(send_pkt.data, token);
                    send_pkt.payload_id = 0;
                    send_pkt.TYPE = 0;
                    send_pkt.payload_size = strlen(send_pkt.data);
                    send_pkt.seq_no = seq;
                    seq+=send_pkt.payload_size;                    
                    if(!send_pkt.isLast)
                        printf(BLUE BOLD "SENT PKT: Seq. No. = %d, Size = %d Bytes\n" RESET, send_pkt.seq_no, send_pkt.payload_size);

                    if (send(sock_fd, &send_pkt, sizeof(send_pkt), 0) == -1) {
                        printError("send()");
                    }
                    
                    // Set timeout timer
                    struct timeval timer;
                    timer.tv_sec = RETRANS_TIMEOUT_SEC;
                    timer.tv_usec = 0;
                    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer)) < 0) {
                        printError("setsockopt");
                    }

                    state = 1;
                    break;
            }
            case 1: {
                    if (recv(sock_fd, &rcv_ack, sizeof(rcv_ack), 0) == -1) {
                        // Handle timeout
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            if(!send_pkt.isLast)
                                printf(YELLOW BOLD "RE-TRANSMIT PKT: Seq. No. = %d, Size = %d Bytes\n" RESET, send_pkt.seq_no, send_pkt.payload_size);
                            if (send(sock_fd, &send_pkt, sizeof(send_pkt), 0) == -1) {
                                printError("send()");
                            }
                            break;
                        }
                        printError("recv()");
                    }
                    // Correct ACK received
                    if (rcv_ack.payload_id==0) { 
                        state = 2;
                        if(!rcv_ack.isLast)
                            printf(GREEN BOLD "RCVD ACK: Seq. No. = %d\n" RESET, rcv_ack.seq_no);
                        else
                            state = 4;  // If ACK to dummy packet is received, don't print but move to close connection
                        break;
                    }
                    // If incorrect ACK received, ignore
                    break;
            }
            case 2: {
                    //Tokenizing the buffer on the basis of commas and periods
                    token = strtok(NULL, ",.");
                    if(token!=NULL) {
                        send_pkt.isLast = false;
                    } else {
                        // If token is NULL, it means the buf is completely tokenized
                        // Send an empty dummy packet to tell server that the file has ended
                        token = "";
                        send_pkt.isLast = true;
                    }
                    strcpy(send_pkt.data, token);
                    send_pkt.payload_id = 1;
                    send_pkt.TYPE = 0;
                    send_pkt.payload_size = strlen(send_pkt.data);
                    send_pkt.seq_no = seq;
                    seq+=send_pkt.payload_size;

                    if(!send_pkt.isLast)
                        printf(BLUE BOLD "SENT PKT: Seq. No. = %d, Size = %d Bytes\n" RESET, send_pkt.seq_no, send_pkt.payload_size);

                    if (send(sock_fd, &send_pkt, sizeof(send_pkt), 0)==-1) {
                        printError("send()");
                    }

                    // Set timeout timer
                    struct timeval timer;
                    timer.tv_sec = RETRANS_TIMEOUT_SEC;
                    timer.tv_usec = 0;
                    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer)) < 0) {
                        printError("setsockopt");
                    }
                    state = 3;
                    break;
            }
            case 3: {
                    if (recv(sock_fd, &rcv_ack, sizeof(rcv_ack), 0) == -1) {
                        // Handle timeout
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            if(!send_pkt.isLast)
                                printf(YELLOW BOLD "RE-TRANSMIT PKT: Seq. No. = %d, Size = %d Bytes\n" RESET, send_pkt.seq_no, send_pkt.payload_size);
                            if (send(sock_fd, &send_pkt, sizeof(send_pkt), 0) == -1) {
                                printError("send()");
                            }
                            break;
                        }
                        printError("recv()");
                    }
                    // Correct ACK received
                    if (rcv_ack.payload_id==1) { 
                        state = 0;
                        if(!rcv_ack.isLast)
                            printf(GREEN BOLD "RCVD ACK: Seq. No. = %d\n" RESET, rcv_ack.seq_no);
                        else
                            state = 4; // If ACK to dummy packet is received, don't print but move to close connection
                        break;
                    }
                    // If incorrect ACK received, ignore
                    break;
            }
            case 4: {
                fflush(stdout);
                fclose(input);
                close(sock_fd);
                exit(0);
            }
        }
    }
    close(sock_fd);
    fclose(input);
    return 0;
}