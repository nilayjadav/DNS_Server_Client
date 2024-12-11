#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define DNS_SERVER "127.0.0.1"  // DNS Server
#define DNS_PORT 8053
#define BUF_SIZE 512
#define TIMEOUT 5

struct DNSHeader {
    unsigned short id;
    unsigned short flags;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
};

struct DNSQuestion {
    unsigned short qtype;
    unsigned short qclass;
};

// Function to format the domain name into the DNS query format
void format_domain_name(char *domain_name, unsigned char *buffer) {
    int length = 0;
    char *token = strtok(domain_name, ".");
    while (token != NULL) {
        buffer[length++] = strlen(token);
        memcpy(buffer + length, token, strlen(token));
        length += strlen(token);
        token = strtok(NULL, ".");
    }
    buffer[length] = 0; // Null terminator
}

// Function to set a timeout for the socket
void set_socket_timeout(int sockfd, int timeout_sec) {
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Setting timeout failed");
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    unsigned char buffer[BUF_SIZE] = {0};
    char input[256];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    set_socket_timeout(sockfd, TIMEOUT);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    server_addr.sin_addr.s_addr = inet_addr(DNS_SERVER);

    while (1) {
        printf("Enter a domain name (or type 'exit' to quit): ");
        scanf("%255s", input);

        if (strcmp(input, "exit") == 0) {
            printf("Exiting DNS client.\n");
            break;
        }

        memset(buffer, 0, BUF_SIZE);

        // Prepare the DNS query
        struct DNSHeader *header = (struct DNSHeader *)buffer;
        header->id = htons(12345); // Arbitrary ID
        header->flags = htons(0x0100); // Standard query
        header->qdcount = htons(1);   // One question

        unsigned char *qname = buffer + sizeof(struct DNSHeader);
        char domain_copy[256];
        strcpy(domain_copy, input); // Make a copy to avoid modifying the original input
        format_domain_name(domain_copy, qname);

        struct DNSQuestion *question = (struct DNSQuestion *)(qname + strlen((char *)qname) + 1);
        question->qtype = htons(1);  // Query type A (IPv4)
        question->qclass = htons(1); // Class IN

        int query_len = sizeof(struct DNSHeader) + strlen((char *)qname) + 1 + sizeof(struct DNSQuestion);

        if (sendto(sockfd, buffer, query_len, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Sendto failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Query sent for domain: %s\n", input);

        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fprintf(stderr, "DNS server timeout. No response received within %d seconds.\n", TIMEOUT);
            } else {
                perror("Recvfrom failed");
            }
            continue;
        }

        printf("DNS response received. Parsing...\n");

        struct DNSHeader *response_header = (struct DNSHeader *)buffer;
        int answer_count = ntohs(response_header->ancount);

        if (answer_count == 0) {
            printf("No resolved IPs found for %s.\n", input);
            continue;
        }

        // Parse the answer section
        unsigned char *reader = buffer + query_len;
        printf("\n+----------------------+-------------------+\n");
        printf("| Domain Name          | Resolved IP       |\n");
        printf("+----------------------+-------------------+\n");
        for (int i = 0; i < answer_count; i++) {
            reader += 12; // Skip the name pointer, type, class, TTL, and RDLENGTH fields
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, reader, ip, INET_ADDRSTRLEN);
            printf("| %-20s | %-17s |\n", input, ip);  // Display domain name as entered
            reader += 4; // Move to the next answer
        }
        printf("+----------------------+-------------------+\n");
    }

    close(sockfd);
    return 0;
}

     

       

