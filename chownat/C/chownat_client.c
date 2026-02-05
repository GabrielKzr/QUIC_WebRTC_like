#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>

#define localport 44444        // any local port
#define remoteaddr "10.0.0.30" // remote public address
#define remoteport 2222        // combined port with remote server
#define size 4096

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

/*
    * Function to try client connection by making a UDP hole punching 
    * forcing the NAT table to create an entry informing src/dst IPs & ports
*/
void client_connect(int chownat, struct sockaddr_in dst) {
    printf("Opening a connection to the remote end\n");
    
    while (1)
    {
        printf("8Attempting to connect\n");

        char* msg = "01\n";

        sendto(chownat, msg, strlen(msg), 0, (struct sockaddr*)&dst, sizeof(dst));
        static char buffer[4];
        recv(chownat, buffer, 3, 0);
        buffer[4] = 0;

        // printf("MENSAGEM RECEBIDA %s", buffer);

        if(strcmp(buffer, "03\n") == 0) {
            sendto(chownat, "03\n", strlen(msg), 0, (struct sockaddr*)&dst, sizeof(dst));
            printf("REMOTE: Connection opened to remote end\n");
            return;
        }
    }  
}

/* 
    * Function to disconnect the client from the remote end
    * (It keeps the program in an infinite loop if the process is not killed)
*/
void chownat_disconnect(int chownat, struct sockaddr_in dst)
{
    printf("DEBUG: 9Attempting to disconnect\n");

    sendto(chownat, "02\n", 3, 0, (struct sockaddr *)&dst, sizeof(dst));
    while (true) {
        printf("DEBUG: Trying to disconnect...\n");
        static char msg[3];
        recv(chownat, msg, 3, 0);
        if (strncmp(msg, "02\n", 3) == 0) {
            sendto(chownat, "02\n", 3, 0, (struct sockaddr *)&dst, sizeof(dst));
            break;
        }
    }
    printf("DEBUG: REMOTE: Disconnected\n");
}

/*
    * Function to set a UDP socket that is going to be used for the UDP hole punching
*/
int client_bind(struct sockaddr_in local) {

    int opt_val = 1;  // Valor para SO_REUSEADDR

    printf("DEBUG: Binding a new socket to %d\n", ntohs(local.sin_port));

    // Criar o socket TCP
    int waitcli = socket(AF_INET, SOCK_STREAM, 0);
    if (waitcli < 0) {
        perror("Erro ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o socket para reutilizar o endereço
    if (setsockopt(waitcli, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) < 0) {
        perror("Erro ao configurar setsockopt");
        close(waitcli);
        exit(EXIT_FAILURE);
    }

    // Fazer o bind do socket ao endereço local
    if (bind(waitcli, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("Erro ao fazer o bind");
        close(waitcli);
        exit(EXIT_FAILURE);
    }

    // Deixar o socket em modo listening (aguardando conexões)
    if (listen(waitcli, 20) < 0) {
        perror("Erro ao escutar");
        close(waitcli);
        exit(EXIT_FAILURE);
    }

    printf("DEBUG: Esperando conexão...\n");

    return waitcli;
}

int main(void) {
    printf("DEBUG: Opening socket on port %d\n", remoteport);

    int chownat = socket(AF_INET, SOCK_DGRAM, 0);
    if (chownat < 0) {
        printf("ERROR: socket %s\n", strerror(errno));
        exit(errno);
    }

    struct sockaddr_in src = {};
    src.sin_family = AF_INET;
    src.sin_port = htons(remoteport);
    src.sin_addr.s_addr = INADDR_ANY;
    if (bind(chownat, (struct sockaddr*)&src, sizeof(src)) < 0) {
        printf("ERROR: bind %s\n", strerror(errno));
        exit(errno);
    }

    struct sockaddr_in local = {};
    local.sin_family = AF_INET;
    local.sin_port = htons(localport);
    local.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct sockaddr_in dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(remoteport);
    dst.sin_addr.s_addr = inet_addr(remoteaddr);

    int waitcli = client_bind(local);
    int sock;

    /*
        * Waits for a TCP request on localport (in this case, 44444)
    */
    while ((sock = accept(waitcli, NULL, NULL)) >= 0)
    {   
        // printf("DEBUG: Conexão aceita. Esperando mensagem...\n");
        
        client_connect(chownat, dst);

        static char buffer[256][size] = {0};
        static size_t sizes[256] = {};
        int closed = 0;
        int id = 0;
        int expected = 0;
        fd_set read_fds;

        while (!closed)
        {
            /*
                * Timeout of 5 seconds for select
            */
            struct timeval timeout = {
                .tv_sec = 0,
                .tv_usec = 1000
            };
            
            FD_ZERO(&read_fds);
            FD_SET(chownat, &read_fds);
            if (sock != -1) // nunca vai ser -1, mas estou verificando de qualquer forma
                FD_SET(sock, &read_fds);
            
            int ready;
            
            /*
                * Waits a message to be received from the two sockets (UDP and TCP)
                * If a message is received, it processes it accordingly
                * If no message is received, it sends a keep-alive message to the remote end
            */
            while ((ready = select(max(chownat, sock) + 1, &read_fds, NULL, NULL, &timeout))) {

                if (ready < 0) {
                    printf("ERROR: select %s\n", strerror(errno));
                    exit(errno);
                } else {
                    printf("DEBUG: some message has been received at %d\n", ready);
                }

                static char command[size];
                if (sock != -1 && FD_ISSET(sock, &read_fds)) {  // pacote TCP           
                    
                    // printf("DEBUG: PACOTE TCP\n");
                    
                    int nbytes = read(sock, command, size);
                    if (nbytes == 0) { // recebeu TCP mas está vazio, então fecha conexão
                        id = 0;
                        expected = 0;

                        printf("DEBUG: REMOTE: 1Attempting to disconnect\n");
                        chownat_disconnect(chownat, dst);

                        close(sock);
                        sock = -1;
                        closed = 1;
                    } else { // recebeu TCP e vai redirecionar para UDP (vai para remoto)
                        // Manter bufferizado
                        // printf("ID: %d\n", id);
                        memcpy(&buffer[id], command, nbytes);
                        sizes[id] = nbytes;
                        printf("DEBUG: Retransmitting packet %d\n", id);
                        static char outbuf[size];
                        outbuf[0] = '0';
                        outbuf[1] = '9';
                        outbuf[2] = id;
                        outbuf[3] = 0;
                        id++;

                        if(id == 256) id = 0;
                    
                        // printf("COPIANDO OutBUF: %s\n", outbuf);
                        memcpy(&outbuf[3], command, nbytes);

                        /*
                        for(int i = 0; i < nbytes+3; i++) {
                            printf("%c", outbuf[i]);
                        }
                        printf("\n");
                        */

                        sendto(chownat, outbuf, nbytes+3, 0, (struct sockaddr*)&dst, sizeof(dst));
                    }
                }

                else if (FD_ISSET(chownat, &read_fds)) { // pacote do chownat UDP

                    // printf("DEBUG: PACOTE UDP\n");

                    int recvd = recv(chownat, command, size-3, 0);
                    if (recvd < 0) { // erro na recepção
                        printf("ERROR: recv %s\n", strerror(errno));
                        exit(errno);
                    }
                    
                    if (recvd < 3) { // se for menor que 3, então é keep-alive, então ignora
                        // Ignore keep-alives
                        printf("DEBUG: keep-alive");
                        FD_ZERO(&read_fds);
                        FD_SET(chownat, &read_fds);
                        if (sock != -1)
                            FD_SET(sock, &read_fds);
                        continue;
                    }

                    if(strncmp(command, "02\n", 3) == 0) { // tentativa de desconexão
                    
                        id = 0;
                        expected = 0;

                        printf("REMOTE: 3Attempting to disconnect\n");
                        chownat_disconnect(chownat, dst);
                        close(sock);
                        sock = -1;

                        closed = 1;

                    } else if(strncmp(command, "03\n", 3) == 0) { // só confirma se está conectado (mensagens duplicadas na conexão)
                        FD_ZERO(&read_fds);
                        FD_SET(chownat, &read_fds);
                        if (sock != -1)
                            FD_SET(sock, &read_fds);
                        continue;

                    } else if(strncmp(command, "08", 2) == 0) { // reenvia caso seja necessário

                        uint8_t got = command[2];

                        printf("DEBUG: Remote host needs packet %d, we're on %d\n", got, id);

                        for (uint8_t i = got; i < id; i++) {
                            static char outbuf[size] = {0};
                            outbuf[0] = '0';
                            outbuf[1] = '9';
                            outbuf[2] = i;
                            memcpy(&outbuf[3], &buffer[i], sizes[i]);
                            sendto(chownat, outbuf, sizes[i]+3, 0, (struct sockaddr*)&dst, sizeof(dst));
                        }     

                    } else if(strncmp(command, "09", 2) == 0) { // recebeu um pacote UDP 

                        uint8_t got = command[2];
                        printf("DEBUG: Got packet %d, expected packet %d\n", got, (uint8_t)(expected));
                        char msg[] = "080";
                        msg[2] = (uint8_t)(expected);

                        if(got != (uint8_t)expected) { // se não for o esperado, pede novamente
                            printf("Asking for packet %d\n", (uint8_t)(expected));
                            sendto(chownat, msg, sizeof(msg), 0, (struct sockaddr *)&dst, sizeof(dst));
                        } else { // se foi o esperado, redireciona para a porta local com TCP
                            if (send(sock, &command[3], recvd - 3, 0) < 0) {
                                printf("ERROR: send %s\n", strerror(errno));
                                exit(errno);
                            }
                            expected++;
                            if(expected == 255) expected = 0;
                        }
                    }
                }  // else 
            } // while select

            // send keep-alive
            int sent = sendto(chownat, NULL, 0, 0, (struct sockaddr *)&dst, sizeof(dst));
            if (sent < 0) {
                printf("ERROR: send %s\n", strerror(errno));
                exit(errno);
            }
            printf("DEBUG: Sent keep-alive\n");
        } // while not closed    
    } // while accept

    close(waitcli);
    close(chownat);
    return 0;
} // main