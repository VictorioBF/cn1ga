#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define ECHOMAX 255

// Função que será executada pelas threads
void *thread_function(void *arg) {
    int sock,i;
	struct sockaddr_in target;
    char linha[ECHOMAX+1];
	char *servIP;

	servIP = "172.17.0.2";

	/* Criando Socket */
	if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    		printf("Socket Falhou!!!\n");
		
	/* Construindo a estrutura de endereco do servidor
	A funcao bzero eh usada para colocar zeros na estrutura target */
    bzero((char *)&target,sizeof(target));
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(servIP); /* host local */
    target.sin_port = htons(6000); /* porta de destino */

	do {
        gets(linha); // irah gerar um warning de unsafe/deprecated.
		/* Envia mensagem para o endereco remoto
		parametros(descritor socket, dados, tamanho dos dados, flag, estrutura do socket remoto, tamanho da estrutura) */
        sendto(sock, linha, ECHOMAX, 0, (struct sockaddr *)&target, sizeof(target));
    } while(strcmp(linha,"exit"));
    close(sock);
    return 0;
}

int main() {
    pthread_t threads[2];
    int thread_args[2];
    int i;

    // Criando duas threads
    for (i = 0; i < 2; i++) {
        thread_args[i] = i;
        pthread_create(&threads[i], NULL, thread_function, (void *)&thread_args[i]);
    }

    // Esperando as threads terminarem
    for (i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Threads finalizadas.\n");
    return 0;
}
