#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include <netdb.h>

#define ECHOMAX 255
#define NUM_IPS 3 // Mínimo para o trabalho
#define IP_SIZE 16
#define APP_PORT 6001

// typedef int bool;
// #define true 1
// #define false 0
// bool error;

// versão 0.20

int interval = 1;
int stop = 0;
char ipAddresses[NUM_IPS][IP_SIZE];
int current_ip_index = 0;

// mutex
pthread_mutex_t input_lock;
pthread_cond_t input_cond;
int pause_output = 0;

// Controle do array de IPs
// Insere um novo IP no array
void add_ip_address(const char *new_ip_address) {
    if (current_ip_index < NUM_IPS) {
        strncpy(ipAddresses[current_ip_index], new_ip_address, IP_SIZE - 1);
        ipAddresses[current_ip_index][IP_SIZE - 1] = '\0'; // Garante que termina com o caractere nulo
        current_ip_index++;
    }
}

// Busca o IP local (127.0.0.1)
void get_local_ip(char *ip_buffer, size_t buffer_size) {
    char hostbuffer[256];
    struct hostent *host_entry;
    int hostname;

    // Pega o hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname == -1) {
        perror("gethostname");
        exit(EXIT_FAILURE);
    }

    // Pega a informação
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL) {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    // Converte para string
    struct in_addr *addr = (struct in_addr *)host_entry->h_addr_list[0];
    strncpy(ip_buffer, inet_ntoa(*addr), buffer_size);
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
    // printf("Comando executado, resultado: \n%s", result);

    pclose(pipe);
    return result;
}

// Funções para as threads de envio e recebimento de pacotes
// Função de envio de informações
void *data_sender_thread(void *arg) {
    int sock,i;
    struct sockaddr_in destiny;
    char linha[ECHOMAX+1];
    char servIP[ECHOMAX];
    FILE *fp;
    char path[1035];

    // Criando Socket
    // printf("Criando socket para o envio...\n");
    if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
        printf("Socket Falhou!!!\n");
        return (void *)1;
    }
    printf("Socket deu boa!!!\n");

    do {
        // Bloqueio por mutex
        pthread_mutex_lock(&input_lock);
        while (pause_output) {
            pthread_cond_wait(&input_cond, &input_lock);
            printf("envio bloqueado");
        }
        pthread_mutex_unlock(&input_lock);
        // Todo: Fazer uma ou algums funções para executar os comandos e montar a string
        /*
        organizar comandos
        - organizar strings
        - definir pacotes
        */
        // Lendo informações para serem enviadas
        // printf("Thread de envio tentantdo executar comando...\n");
        strcpy(path, execute_command("vmstat"));

        // printf("Resultado do comando %s\n", path);
        // Envio para os múltiplos endereços
        for (size_t i = 0; i < current_ip_index; i++) {
            const char* ipAddress = ipAddresses[i];
            destiny.sin_family = AF_INET;
            destiny.sin_addr.s_addr = inet_addr(ipAddress); // Endereço
            destiny.sin_port = htons(APP_PORT); // Porta de destino
            sendto(sock, path, ECHOMAX, 0, (struct sockaddr *)&destiny, sizeof(destiny)); // Envio das informações
            // printf("Pacote enviado\n");
        }
        sleep(interval);
    } while(stop == 0);
    // printf("Fechando socket de envio\n");
    close(sock);

    return (void *)0; // É necessário cast para evitar warning
}

// Função que rebe as informações
void *data_receiver_thread(void *arg) {
    int sock;
    // Estrutura: familia + endereco IP + porta
    struct sockaddr_in me, from;
    int adl = sizeof(me);
    char linha[ECHOMAX];

    // Cria o socket para enviar e receber datagramas parâmetros(familia, tipo, protocolo)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("ERRO na Criacao do Socket!\n");
    else  
        printf("Servidor esperando mensagens...\n");

    // Construcao da estrutura do endereco local, preenchendo a estrutura socket me (familia, IP, porta)
    me.sin_family = AF_INET;
    me.sin_addr.s_addr=htonl(INADDR_ANY); // Endereco IP local
    me.sin_port = htons(APP_PORT); // Porta local

    // Bind para o endereco local parametros(descritor socket, estrutura do endereco local, comprimento do endereco)
    if(-1 != bind(sock, (struct sockaddr *)&me, sizeof(me)))
        do  {
            // Bloqueio por mutex
            pthread_mutex_lock(&input_lock);
            while (pause_output) { pthread_cond_wait(&input_cond, &input_lock); }
            pthread_mutex_unlock(&input_lock);
            /* Recebe mensagem do endereco remoto
            parametros(descritor socket, dados, tamanho dos dados, flag, estrutura do socket remoto, tamanho da estrutura) */
            recvfrom(sock, linha, ECHOMAX, 0, (struct sockaddr *)&from, &adl);
            printf("%s", linha);
        }
        while(stop == 0);
    else puts("Porta ocupada");

    // Fecha o socket
    close(sock);
    return 0;
}

//
void alter_interval(){
    // mutex

    return (void)0;
}

// Controle de input do usuário
void *user_input_thread(void *arg) {
    char *input;
    char in[100];

    do {
        fgets(in, sizeof(in), stdin);
        
        pthread_mutex_lock(&input_lock);
        pause_output = 1;  // Signal the output thread to pause
        pthread_mutex_unlock(&input_lock);

        // Bloqueia a execução até o usuário apertar enter
        printf("1: Adicionar IP | 2: Alterar tempo de envio | 0: Finalizar programa \nDigite um número:\n");
        *input = getchar();
        switch (*input) {
        case '1':
            // code block
            break;
        case '2':
            // code block
            break;
        case '0':
            stop = 1;
            break;
        default:
            // code block
        }
        pthread_mutex_lock(&input_lock);
        pause_output = 0;  // Signal the output thread to resume
        pthread_cond_signal(&input_cond);
        pthread_mutex_unlock(&input_lock);
    } while (stop == 0);
    return (void*)0;
}

int main() {
    pthread_t threads[2];
    int thread_args[2];
    int i;
    char local_ip[100];

    get_local_ip(local_ip, sizeof(local_ip));
    add_ip_address(local_ip);

    // // Inicialização dos mutex
    // pthread_mutex_init(&send, NULL);
    // pthread_mutex_init(&receive, NULL);
    // pthread_mutex_init(&input, NULL);

    i = 0;
    thread_args[i] = i;
    pthread_create(&threads[i], NULL, data_receiver_thread, (void *)&thread_args[i]);
    printf("Thread de recebimento de dados criada.\n");

    i = 1;
    thread_args[i] = i;
    pthread_create(&threads[i], NULL, data_sender_thread, (void *)&thread_args[i]);
    printf("Thread de envio criada.\n");

    // i = 2;
    // thread_args[i] = i;
    // pthread_create(&threads[i], NULL, user_input_thread, (void *)&thread_args[i]);
    // printf("Thread de input criada\n");

    // printf("Juntando 1ª thread\n");
    pthread_join(threads[0], NULL);
    // printf("Juntando 2ª thread\n");
    pthread_join(threads[1], NULL);
    // printf("Juntando 3ª thread\n");
    // pthread_join(threads[2], NULL);

    printf("Programa finalizado!\n");
    return 0;
}

/*

input do usuário
- alteração de tempo de coleta dos dados
- inclusão de novos ips 

*/