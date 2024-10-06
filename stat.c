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
#define NUM_IPS 3 // Mínimo para o trabalho

// typedef int bool;
// #define true 1
// #define false 0
// bool error;

// versão 0.15

int interval = 1;
int segue_o_baile = 1;
char *ipAddresses[NUM_IPS] = {NULL};

// Controle do array de IPs
// Insere um novo IP no array
void add_ip_address(char new_ip_address) {
    int nextPosition = -1;

    // Busca a próxima posição vazia
    for (size_t i = 0; i < sizeof(ipAddresses) / sizeof(int); i++) {
        if (ipAddresses[i] == NULL) {
            nextPosition = i;
            break;
        }
    }

    // Checa para ver se é possível adicionar
    if (nextPosition != -1) { // -1 quando não é possível (array já está preenchido)
        *ipAddresses[nextPosition] = new_ip_address;
    }
}

// Recebe um comando e retorna a última linha dele
char *execute_command(const char *command) {
    char buffer[128];
    char *result = NULL;
    size_t size = 1;
    FILE *pipe = popen(command, "r");

    if (!pipe) {
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t len = strlen(buffer);
        char *new_result = realloc(result, size + len);
        if (new_result == NULL) {
            free(result);
            pclose(pipe);
            return NULL;
        }
        result = new_result;
        strcpy(result, buffer);
        size = len + 1;
    }
    printf("Comando executado, resultado: \n%s", result);

    pclose(pipe);
    return result;
}

// Funções para as threads de envio e recebimento de pacotes
// Função de envio de informações
void *sender_thread(void *arg) {
    int sock,i;
    struct sockaddr_in destiny;
    char linha[ECHOMAX+1];
    char servIP[ECHOMAX];
    FILE *fp;
    char path[1035];

    // Criando Socket
    printf("Criando socket para o envio...\n");
    if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
        printf("Socket Falhou!!!\n");
    printf("Socket deu boa!!!\n");

    do {
        // Todo: Fazer uma ou algums funções para executar os comandos e montar a string
        /*
        organizar comandos
        - organizar strings
        - definir pacotes
        */
        // Lendo informações para serem enviadas
        printf("Thread de envio tentantdo executar comando...\n");
        strcpy(path, execute_command("vmstat"));

        printf("Resultado do comando %s\n", path);
        // Envio para os múltiplos endereços
        for (size_t i = 0; i < sizeof(ipAddresses) / sizeof(int); i++) {
            destiny.sin_family = AF_INET;
            destiny.sin_addr.s_addr = inet_addr("127.0.0.1"); // Endereço
            destiny.sin_port = htons(6000); // Porta de destino
            sendto(sock, path, ECHOMAX, 0, (struct sockaddr *)&destiny, sizeof(destiny)); // Envio das informações
        }
        sleep(interval);
    } while(segue_o_baile = 1);
    close(sock);

    return (void *)0; // É necessário cast para evitar warning
}

// Função que rebe as informações
void *receiver_thread(void *arg) {
    int sock;
    /* Estrutura: familia + endereco IP + porta */
    struct sockaddr_in me, from;
    // socklen_t for MAC
    int adl = sizeof(me);
    char linha[ECHOMAX];

    // Cria o socket para enviar e receber datagramas parâmetros(familia, tipo, protocolo)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("ERRO na Criacao do Socket!\n");
    else  
        printf("Servidor esperando mensagens...\n");

    // Construcao da estrutura do endereco local, Preenchendo a estrutura socket me (familia, IP, porta)
    me.sin_family = AF_INET;
    me.sin_addr.s_addr=htonl(INADDR_ANY); /* endereco IP local */
    me.sin_port = htons(6000); /* porta local  */

    // Bind para o endereco local parametros(descritor socket, estrutura do endereco local, comprimento do endereco)
    if(-1 != bind(sock, (struct sockaddr *)&me, sizeof(me)))
    do  {
        /* Recebe mensagem do endereco remoto
        parametros(descritor socket, dados, tamanho dos dados, flag, estrutura do socket remoto, tamanho da estrutura) */
        recvfrom(sock, linha, ECHOMAX, 0, (struct sockaddr *)&from, &adl);
        printf("recebido: %s\n", linha);
    }
    while(segue_o_baile = 1);
    else puts("Porta ocupada");
    close(sock);
    return 0;
}

int main() {
    pthread_t threads[2];
    int thread_args[2];
    int i;

    interval = 2;

    i = 0;
    thread_args[i] = i;
    pthread_create(&threads[i], NULL, receiver_thread, (void *)&thread_args[i]);
    printf("Thread de recebimento criada.\n");

    i = 1;
    thread_args[i] = i;
    pthread_create(&threads[i], NULL, sender_thread, (void *)&thread_args[i]);
    printf("Thread de envio criada.\n");

    printf("Juntando 1ª thread\n");
    pthread_join(threads[0], NULL);
    printf("Juntando 2ª thread\n");
    pthread_join(threads[1], NULL);

    printf("Programa finalizado!\n");
    return 0;
}

/*

input do usuário
- alteração de tempo de coleta dos dados
- inclusão de novos ips 

*/