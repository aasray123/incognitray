// To run
// gcc /home/aasray/Documents/incognitray/listener.c -o
// /home/aasray/Documents/incognitray/listener -lcurl sudo nmcli connection up
// "GNXS-5G-376260"

#include <arpa/inet.h>
#include <curl/curl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct ResponseData {
    unsigned char data[512];
    size_t size;
};

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct ResponseData *mem = (struct ResponseData *)userp;

    // Prevent buffer overflow (standard UDP DNS limit is 512 bytes)
    if (mem->size + realsize <= 512) {
        memcpy(&(mem->data[mem->size]), contents, realsize);
        mem->size += realsize;
    }
    return realsize;
}

int main(int argc, char *argv[]) {
    // Initialize Curl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    struct addrinfo hints, *res, *p;
    int status;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo("0.0.0.0", "53", &hints, &res)) != 0) {
        return 2;
    }

    // INITIALIZE THE SOCKETS
    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // BIND THE SOCKET TO THE ADDRESS GIVEN ABOVE IN getaddrinfo
    // bind(int fd, const struct sockaddr *addr, socklen_t len)
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Bind failed");
        return 1;
    }
    freeaddrinfo(res);

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
    unsigned char buffer[512];

    // LOOP TO CAPTURE EVER DNS PACKET
    while (1) {
        int numbytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr *)&their_addr, &addr_len);

        if (numbytes == -1) {
            perror("recvfrom failed");
            continue;
        }
        printf("Successfully received a %d byte DNS query!\n", numbytes);

        unsigned short transaction_id = (buffer[0] << 8) | buffer[1];
        printf("Transaction ID: 0x%04X\n", transaction_id);

        printf("Requested Domain: ");
        int i = 12;
        // ITERATE THROUGH BUFFER TO FIGURE OUT WHAT IS BEING ASKED
        while (buffer[i] != 0 && i < numbytes) {
            int label_len = buffer[i];

            for (int j = 0; j < label_len; j++) {
                printf("%c", buffer[i + 1 + j]);
            }
            printf(".");

            i += label_len + 1;
        }
        printf("\n");

        // Initialize a new curl session for this specific request
        CURL *curl = curl_easy_init();
        if (curl) {
            // Set the required DoH HTTP headers
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(
                headers, "Content-Type: application/dns-message");

            headers =
                curl_slist_append(headers, "Accept: application/dns-message");

            // Prepare our response container
            struct ResponseData doh_response = {.size = 0};

            // Configure the curl request
            curl_easy_setopt(curl, CURLOPT_URL,
                             "https://cloudflare-dns.com/dns-query");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Pass the raw DNS UDP packet as the POST body
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, numbytes);

            // Tell curl where to save the response
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &doh_response);

            // Hardcode the IP for cloudflare-dns.com so libcurl skips the DNS
            // lookup
            struct curl_slist *resolve_hosts = NULL;
            resolve_hosts = curl_slist_append(resolve_hosts,
                                              "cloudflare-dns.com:443:1.1.1.1");
            curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_hosts);
            // Execute the request
            CURLcode res = curl_easy_perform(curl);

            if (res == CURLE_OK && doh_response.size > 0) {
                // Send the exact binary response back to the waiting local
                // client (e.g., dig)
                sendto(sockfd, doh_response.data, doh_response.size, 0,
                       (struct sockaddr *)&their_addr, addr_len);
                printf("Forwarded %zu bytes back to client!\n",
                       doh_response.size);
            } else {
                fprintf(stderr, "cURL error: %s\n", curl_easy_strerror(res));
            }
			printf("\n");
            // Clean up the session
            curl_slist_free_all(headers);
			curl_slist_free_all(resolve_hosts);
            curl_easy_cleanup(curl);
        }
    }

    close(sockfd);
    curl_global_cleanup();
    return 0;
}
