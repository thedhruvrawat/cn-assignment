/*
    FILE:   s1.c
    RUN:    gcc s1.c -o s1
            ./s1
    NAME:   Dhruv Rawat
    ID:     2019B3A70537P
    SEC:    P2
*/

#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define BUFLEN 10000    // Max length of buffer
#define PORT 8537       // The port on which to listen for incoming data
#define PDR 10          // Packet Drop Rate in percentage. Currently set at 10%


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

void printError(char *s) {
    perror(s);
    exit(1);
}

typedef struct packet {
    int payload_id;
    int seq_no;
    char data[BUFLEN];
    int payload_size;
    int TYPE;       // 0 for data, 1 for ACK
    bool isLast;    // To check if sent data packet is last packet
} PKT;

int main(void) {
    struct sockaddr_in server_addr, cl1_addr, cl2_addr;
    int sock_fd, recv_len, opt=1;
    int addrlen = sizeof(server_addr);
    int c1_fd, c2_fd;
    PKT rcv_pkt, ack_pkt;

    srand(time(0));     // Seeding the random number generator

    // Socket creation
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printError("socket");
    }

    // Set socket options
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        printError("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Initializing socket
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind server socket to address
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printError("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    if (listen(sock_fd, 2) < 0) {
        printError("listen");
        exit(EXIT_FAILURE);
    }

    // Accept client c1
    if ((c1_fd = accept(sock_fd, (struct sockaddr *)&cl1_addr, (socklen_t*)&addrlen)) < 0) {
        printError("accept c1");
        exit(EXIT_FAILURE);
    } else {
        printf("Connection accepted from Client 1\n");
    }

    // Accept client c2
    if ((c2_fd = accept(sock_fd, (struct sockaddr *)&cl2_addr, (socklen_t*)&addrlen)) < 0) {
        printError("accept c2");
        exit(EXIT_FAILURE);
    } else {
        printf("Connection accepted from Client 2\n");
    }

    //Open file to write data to
    FILE *output = fopen("list.txt", "w");

    if (output == NULL) {
        printError("Error opening file");
        exit(EXIT_FAILURE);
    }    

    // Initial state is 0
    int state = 0;

    while(1) {
        switch(state) { 
            case 0: { // Receive ID 0 packet from client 1
                fflush(stdout);
                if ((recv_len = recv(c1_fd, &rcv_pkt, sizeof(struct packet), 0)) == -1) {
                    printError("recv()");
                }
                // In-order packet received: Process it
                if (rcv_pkt.payload_id==0 && rcv_pkt.TYPE==0) { 
                    // Randomly drop the packet
                    if (rand() % 100 < PDR) {
                        printf(RED BOLD "DROP PKT [c1]: Seq. No. = %d\n" RESET, rcv_pkt.seq_no);
                        break;
                    }
                    if(!rcv_pkt.isLast)
                        printf(GREEN BOLD "RCVD PKT [c1]: Seq. No. = %d, Size = %d Bytes\n" RESET, rcv_pkt.seq_no, rcv_pkt.payload_size);
                    fprintf(output, "%s", rcv_pkt.data);
                    ack_pkt.payload_id = 0;
                    ack_pkt.isLast = false;
                    ack_pkt.seq_no = rcv_pkt.seq_no;
                    ack_pkt.payload_size = 0;
                    ack_pkt.TYPE = 1;
                    if(rcv_pkt.isLast) {
                        ack_pkt.isLast = true;
                        // Send ACK to other client as well since this is the last dummy packet. Both files over.
                        if (send(c2_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                            printError("send()");
                        }
                    }
                    if(!ack_pkt.isLast)
                        printf(BLUE BOLD "SENT ACK [c1]: Seq. No. = %d\n" RESET, ack_pkt.seq_no);
                    if (send(c1_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                        printError("send()");
                    }
                    // ACK sent, move to next state
                    state = 1;
                    if(!rcv_pkt.isLast) {
                        fprintf(output, ",");
                    } else {
                        state = 4;
                    }
                    break;
                }
                // Out-of order packet received: Ignore it
                break;
            }
            case 1: { // Receive ID 0 packet from client 2
                fflush(stdout);
                if ((recv_len = recv(c2_fd, &rcv_pkt, sizeof(struct packet), 0)) == -1) {
                    printError("recv()");
                }
                // In-order packet received: Process it
                if (rcv_pkt.payload_id==0 && rcv_pkt.TYPE==0) {                     
                    // Randomly drop the packet
                    if (rand() % 100 < PDR) {
                        printf(RED BOLD "DROP PKT [c2]: Seq. No. = %d\n" RESET, rcv_pkt.seq_no);
                        break;
                    }                    
                    if(!rcv_pkt.isLast)
                        printf(GREEN BOLD "RCVD PKT [c2]: Seq. No. = %d, Size = %d Bytes\n" RESET, rcv_pkt.seq_no, rcv_pkt.payload_size);
                    fprintf(output, "%s", rcv_pkt.data);
                    ack_pkt.payload_id = 0;
                    ack_pkt.isLast = false;
                    ack_pkt.seq_no = rcv_pkt.seq_no;
                    ack_pkt.payload_size = 0;
                    ack_pkt.TYPE = 1;
                    if(rcv_pkt.isLast) {
                        ack_pkt.isLast = true;
                        // Send ACK to other client as well since this is the last dummy packet. Both files over.
                        if (send(c1_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                            printError("sendto()");
                        }
                    }
                    if(!ack_pkt.isLast)
                        printf(BLUE BOLD "SENT ACK [c2]: Seq. No. = %d\n" RESET, ack_pkt.seq_no);
                    if (send(c2_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                        printError("sendto()");
                    }
                    // ACK sent, move to next state
                    state = 2;
                    if(!rcv_pkt.isLast) {
                        fprintf(output, ",");
                    } else {
                        state = 4;
                    }
                    break;
                }
                // Out-of order packet received: Ignore it
                break;
            }
            case 2: { // Receive ID 1 packet from client 1
                fflush(stdout);
                if ((recv_len = recv(c1_fd, &rcv_pkt, sizeof(struct packet), 0)) == -1) {
                    printError("recv()");
                }
                // In-order packet received: Process it
                if (rcv_pkt.payload_id==1 && rcv_pkt.TYPE==0) {                     
                    // Randomly drop the packet
                    if (rand() % 100 < PDR) {
                        printf(RED BOLD "DROP PKT [c1]: Seq. No. = %d\n" RESET, rcv_pkt.seq_no);
                        break;
                    }
                    if(!rcv_pkt.isLast)
                        printf(GREEN BOLD "RCVD PKT [c1]: Seq. No. = %d, Size = %d Bytes\n" RESET, rcv_pkt.seq_no, rcv_pkt.payload_size);
                    fprintf(output, "%s", rcv_pkt.data);
                    ack_pkt.payload_id = 1;
                    ack_pkt.isLast = false;
                    ack_pkt.seq_no = rcv_pkt.seq_no;
                    ack_pkt.payload_size = 0;
                    ack_pkt.TYPE = 1;
                    if(rcv_pkt.isLast) {
                        ack_pkt.isLast = true;
                        // Send ACK to other client as well since this is the last dummy packet. Both files over.
                        if (send(c2_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                            printError("send()");
                        }
                    }
                    if(!ack_pkt.isLast)
                        printf(BLUE BOLD "SENT ACK [c1]: Seq. No. = %d\n" RESET, ack_pkt.seq_no);
                    if (send(c1_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                        printError("send()");
                    }
                    // ACK sent, move to next state
                    state = 3;
                    if(!rcv_pkt.isLast) {
                        fprintf(output, ",");
                    } else {
                        state = 4;
                    }
                    break;
                }
                // Out-of order packet received: Ignore it
                break;
            }
            case 3: { // Receive ID 1 packet from client 2
                fflush(stdout);
                if ((recv_len = recv(c2_fd, &rcv_pkt, sizeof(struct packet), 0)) == -1) {
                    printError("recv()");
                }

                // In-order packet received: Process it
                if (rcv_pkt.payload_id==1 && rcv_pkt.TYPE==0) { 
                    // Randomly drop the packet
                    if (rand() % 100 < PDR) {
                        printf(RED BOLD "DROP PKT [c2]: Seq. No. = %d\n" RESET, rcv_pkt.seq_no);
                        break;
                    }
                    if(!rcv_pkt.isLast)
                        printf(GREEN BOLD "RCVD PKT [c2]: Seq. No. = %d, Size = %d Bytes\n" RESET, rcv_pkt.seq_no, rcv_pkt.payload_size);
                    fprintf(output, "%s", rcv_pkt.data);
                    ack_pkt.payload_id = 1;
                    ack_pkt.isLast = false;
                    ack_pkt.seq_no = rcv_pkt.seq_no;
                    ack_pkt.payload_size = 0;
                    ack_pkt.TYPE = 1;
                    if(rcv_pkt.isLast) {
                        ack_pkt.isLast = true;
                        // Send ACK to other client as well since this is the last dummy packet. Both files over.
                        if (send(c1_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                            printError("send()");
                        }
                    }
                    if(!ack_pkt.isLast)
                        printf(BLUE BOLD "SENT ACK [c2]: Seq. No. = %d\n" RESET, ack_pkt.seq_no);
                    if (send(c2_fd, &ack_pkt, sizeof(struct packet), 0) == -1) {
                        printError("send()");
                    }
                    // ACK sent, move to next state
                    state = 0;
                    if(!rcv_pkt.isLast) {
                        fprintf(output, ",");
                    } else {
                        state = 4;
                    }
                    break;
                }
                // Out-of order packet received: Ignore it
                break;
            }
            case 4: {
                // For replacing the last comma with a period
                fseek(output, -1, SEEK_END);
                fprintf(output, ".");
                fflush(stdout);
                fclose(output);
                close(c1_fd);
                close(c2_fd);
                close(sock_fd);
                exit(0);
            }
        }
    }
    fclose(output);
    close(c1_fd);
    close(c2_fd);
    close(sock_fd);
    return 0;
}