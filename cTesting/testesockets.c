#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include <stdlib.h>

#define ECHOMAX 255

typedef int bool;
#define true 1
#define false 0

bool error;

// Função que será executada pelas threads
void *thread_client(void *arg) {
    int sock,i;
    struct sockaddr_in destiny;
    char linha[ECHOMAX+1];
    char servIP[ECHOMAX];
    FILE *fp;

    // Solicita ao usuário que insira o IP do servidor
    // printf("Por favor, insira o IP do servidor: ");
    // scanf("%s", servIP);

    /* Criando Socket */
    if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
        printf("Socket Falhou!!!\n");
        
    /* Construindo a estrutura de endereco do servidor */
    // bzero((char *)&destiny,sizeof(destiny));
    destiny.sin_family = AF_INET;
    destiny.sin_addr.s_addr = inet_addr("127.0.0.1"); /* host local */
    destiny.sin_port = htons(6000); /* porta de destino */

    do {
        printf("%s", "Digite.\n");
        fgets(linha, 20, fp); // irah gerar um warning de unsafe/deprecated.
        /* Envia mensagem para o endereco remoto
        parametros(descritor socket, dados, tamanho dos dados, flag, estrutura do socket remoto, tamanho da estrutura) */
        sendto(sock, linha, ECHOMAX, 0, (struct sockaddr *)&destiny, sizeof(destiny));
    } while(strcmp(linha,"exit"));
    close(sock);
    return 0;
}

void *thread_server(void *arg) {
    int sock;
    /* Estrutura: familia + endereco IP + porta */
    struct sockaddr_in me, from;
    // socklen_t for MAC
    int adl = sizeof(me);
    char linha[ECHOMAX];

       /* Cria o socket para enviar e receber datagramas
    parâmetros(familia, tipo, protocolo) */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("ERRO na Criacao do Socket!\n");
    else  
        printf("Servidor esperando mensagens...\n");

    /* Construcao da estrutura do endereco local
    Preenchendo a estrutura socket me (familia, IP, porta)*/
    me.sin_family = AF_INET;
    me.sin_addr.s_addr=htonl(INADDR_ANY); /* endereco IP local */
    me.sin_port = htons(6000); /* porta local  */

       /* Bind para o endereco local
    parametros(descritor socket, estrutura do endereco local, comprimento do endereco) */
    if(-1 != bind(sock, (struct sockaddr *)&me, sizeof(me)))
    do  {
        /* Recebe mensagem do endereco remoto
        parametros(descritor socket, dados, tamanho dos dados, flag, estrutura do socket remoto, tamanho da estrutura) */
        recvfrom(sock, linha, ECHOMAX, 0, (struct sockaddr *)&from, &adl);
        printf("%s\n", linha);
    }
    while(strcmp(linha,"exit"));
    else puts("Porta ocupada");
    close(sock);
    return 0;
}

int main() {
    pthread_t threads[2];
    int thread_args[2];
    int i;

    i = 0;
    thread_args[i] = i;
    pthread_create(&threads[i], NULL, thread_server, (void *)&thread_args[i]);
    printf("Thread servidor criada.\n");

    i = 1;
    thread_args[i] = i;
    pthread_create(&threads[i], NULL, thread_client, (void *)&thread_args[i]);
    printf("Thread cliente criada.\n");

    // Esperando as threads terminarem
    for (i = 0; i < 2; i++) {
        printf("Juntando threads\n");
        pthread_join(threads[i], NULL);
    }

    printf("Threads finalizadas.\n");
    return 0;
}
