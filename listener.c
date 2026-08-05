#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res, *p;
    int status;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo("127.0.0.1", "53", &hints, &res)) != 0) {
        return 2;
    }

    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // bind(int fd, const struct sockaddr *addr, socklen_t len)
    bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Bind failed");
        return 1;
    }
    freeaddrinfo(res);

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
    unsigned char buffer[512];

    while (1) {
        int numbytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr *)&their_addr, &addr_len);

        if (numbytes != -1){
			perror("recvfrom failed");
			continue;
		}
		printf("Successfully received a %d byte DNS query!\n", numbytes);
    }

    close(sockfd);
    return 0;
}
