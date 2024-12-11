#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define DNS_PORT 8053
#define BUF_SIZE 512
#define MAX_IPS_PER_DOMAIN 5
#define MAX_DOMAINS 10

struct DNSHeader {
    unsigned short id;
    unsigned short flags;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
};

// Define a domain-to-IP mapping
struct DomainIPMapping {
    char domain[256];
    char ips[MAX_IPS_PER_DOMAIN][16];
    int ip_count; // Number of IPs for this domain
};

struct DomainIPMapping domain_ip_table[MAX_DOMAINS] = {
    {"google.com", {"8.8.8.8", "8.8.4.4"}, 2},
    {"amazon.com", {"54.239.28.85", "54.239.28.84"}, 2},
    {"github.com", {"140.82.112.3", "140.82.114.4"}, 2},
    {"example.com", {"93.184.216.34"}, 1},
    {"localhost", {"127.0.0.1"}, 1},
    {"microsoft.com", {"13.77.161.179", "40.112.72.205"}, 2},
    {"apple.com", {"17.172.224.47", "17.172.224.48"}, 2},
    {"facebook.com", {"31.13.71.36", "31.13.71.37"}, 2},
    {"twitter.com", {"104.244.42.129", "104.244.42.65"}, 2},
    {"linkedin.com", {"108.174.10.10", "108.174.10.11"}, 2}
};

int domain_ip_table_size = MAX_DOMAINS;

// Function to parse the domain name from the query
void parse_domain_name(unsigned char *query, char *domain_name) {
    int len = *query++;
    while (len > 0) {
        strncat(domain_name, (char *)query, len);
        query += len;
        len = *query++;
        if (len > 0) {
            strcat(domain_name, ".");
        }
    }
}

// Function to normalize the domain name by removing prefixes
void normalize_domain_name(char *domain_name) {
    char *start = domain_name;

    if (strncmp(domain_name, "http://", 7) == 0) {
        start = domain_name + 7;
    } else if (strncmp(domain_name, "https://", 8) == 0) {
        start = domain_name + 8;
    }

    if (strncmp(start, "www.", 4) == 0) {
        start += 4;
    }

    // Shift the normalized domain name back to the original variable
    if (start != domain_name) {
        memmove(domain_name, start, strlen(start) + 1);
    }
}

// Function to find the IPs for a given domain name in the mapping table
struct DomainIPMapping *find_mapping_for_domain(const char *domain_name) {
    for (int i = 0; i < domain_ip_table_size; i++) {
        if (strcmp(domain_name, domain_ip_table[i].domain) == 0) {
            return &domain_ip_table[i];
        }
    }
    return NULL; // Domain not found
}

// Function to handle DNS query
void handle_dns_query(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, unsigned char *buffer, int n) {
    printf("Received DNS query from %s:%d\n",
           inet_ntoa(client_addr->sin_addr),
           ntohs(client_addr->sin_port));

    // Parse the DNS header
    struct DNSHeader *request_header = (struct DNSHeader *)buffer;

    // Read the query section
    unsigned char *query_name = buffer + sizeof(struct DNSHeader);
    char domain_name[256] = {0};
    parse_domain_name(query_name, domain_name);

    // Normalize the domain name
    normalize_domain_name(domain_name);

    printf("Domain requested: %s\n", domain_name);

    // Find the IPs for the requested domain
    struct DomainIPMapping *mapping = find_mapping_for_domain(domain_name);
    if (!mapping) {
        printf("Domain not found: %s\n", domain_name);
        // If domain is not found, set error response
        struct DNSHeader *response_header = (struct DNSHeader *)buffer;
        response_header->flags = htons(0x8183); // Name error
        if (sendto(sockfd, buffer, n, 0, (struct sockaddr *)client_addr, client_len) < 0) {
            perror("Sendto failed");
        }
        return;
    }

    // Prepare the response
    unsigned char response[BUF_SIZE] = {0};
    memcpy(response, buffer, n); // Copy the original query for response

    struct DNSHeader *response_header = (struct DNSHeader *)response;
    response_header->flags = htons(0x8180); // Standard response, no error
    response_header->ancount = htons(mapping->ip_count); // Number of answers

    // Move to the answer section
    unsigned char *answer = response + n;
    for (int i = 0; i < mapping->ip_count; i++) {
        // Answer: Type A, Class IN, TTL 60, RDLENGTH 4 (IP address)
        *answer++ = 0xc0; *answer++ = 0x0c;         // Pointer to the query name
        *answer++ = 0x00; *answer++ = 0x01;         // Type A
        *answer++ = 0x00; *answer++ = 0x01;         // Class IN
        *answer++ = 0x00; *answer++ = 0x00; *answer++ = 0x00; *answer++ = 0x3c; // TTL 60
        *answer++ = 0x00; *answer++ = 0x04;         // RDLENGTH 4

        // Convert resolved IP to bytes
        struct in_addr ip_addr;
        inet_pton(AF_INET, mapping->ips[i], &ip_addr);
        memcpy(answer, &ip_addr, 4);
        answer += 4;
    }

    int response_len = answer - response;

    // Send the response
    if (sendto(sockfd, response, response_len, 0, (struct sockaddr *)client_addr, client_len) < 0) {
        perror("Sendto failed");
    } else {
        printf("Response sent to %s:%d\n",
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    unsigned char buffer[BUF_SIZE];
    socklen_t client_len = sizeof(client_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("DNS Server is running and listening on port %d\n", DNS_PORT);

    while (1) {
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("Recvfrom failed");
            continue;
        }

        handle_dns_query(sockfd, &client_addr, client_len, buffer, n);
    }

    close(sockfd);
    return 0;
}

