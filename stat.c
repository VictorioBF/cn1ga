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

// versão 0.40

int interval = 1;
int stop = 0;
char ipAddresses[NUM_IPS][IP_SIZE];
int current_ip_index = 0;

// mutex
pthread_mutex_t interval_lock;
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
    } else {
        printf("Limite de IPs alcançado. Não é possível adicionar mais.\n");
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
    strncpy(ip_buffer, inet_ntoa(*addr), buffer_size - 1);
    ip_buffer[buffer_size - 1] = '\0'; // Garante a terminação nula
}

// Recebe um comando e retorna o resultado completo
char *execute_command(const char *command) {
    char buffer[128];
    char *result = NULL;
    size_t size = 0;
    FILE *pipe = popen(command, "r");

    if (!pipe) {
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t len = strlen(buffer);
        char *new_result = realloc(result, size + len + 1); // +1 para o terminador nulo
        if (new_result == NULL) {
            free(result);
            pclose(pipe);
            return NULL;
        }
        result = new_result;
        strcpy(result + size, buffer);
        size += len;
    }

    pclose(pipe);
    return result;
}

// Função para alterar o intervalo de envio
void alter_interval() {
    char input_buffer[100];
    int new_interval;

    // Todo: fazer envio e recebimento de pacotes que alterem o intervalo dos outros computadores

    printf("Digite o novo intervalo de envio (em segundos): ");

    if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
        // Remove o caractere de nova linha
        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        // Tenta converter a entrada para um inteiro
        new_interval = atoi(input_buffer);

        if (new_interval > 0) {
            pthread_mutex_lock(&interval_lock);
            interval = new_interval;
            pthread_mutex_unlock(&interval_lock);
            printf("Intervalo atualizado para %d segundos.\n", interval);
        } else {
            printf("Entrada inválida! O intervalo deve ser um número inteiro positivo.\n");
        }
    } else {
        printf("Erro ao ler a entrada.\n");
    }
}

// Funções para as threads de envio e recebimento de pacotes
// Função de envio de informações
void *data_sender_thread(void *arg) {
    int sock;
    struct sockaddr_in destiny;
    char path[1035];

    // Criando Socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Socket Falhou!!!\n");
        return (void *)1;
    }
    printf("Socket deu boa!!!\n");

    do {
        // Bloqueio por mutex
        pthread_mutex_lock(&input_lock);
        while (pause_output) {
            pthread_cond_wait(&input_cond, &input_lock);
        }
        pthread_mutex_unlock(&input_lock);

        // Execução dos comandos com dados
        // Todo: alterar para usar os comandos eBPF
        // Todo: alterar para que as string impressas fiquem decentes no output.
        char *command_result = execute_command("vmstat");
        if (command_result != NULL) {
            strncpy(path, command_result, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0'; // Garante a terminação nula
            free(command_result);
        } else {
            fprintf(stderr, "Erro ao executar o comando.\n");
            path[0] = '\0'; // Ou alguma string padrão
        }

        // Envio para os múltiplos endereços
        for (size_t i = 0; i < current_ip_index; i++) {
            const char *ipAddress = ipAddresses[i];
            destiny.sin_family = AF_INET;
            destiny.sin_addr.s_addr = inet_addr(ipAddress); // Endereço
            destiny.sin_port = htons(APP_PORT); // Porta de destino
            sendto(sock, path, strlen(path), 0, (struct sockaddr *)&destiny, sizeof(destiny)); // Envio das informações
        }

        // Leitura thread-safe do intervalo
        pthread_mutex_lock(&interval_lock);
        int current_interval = interval;
        pthread_mutex_unlock(&interval_lock);

        sleep(current_interval);
    } while (stop == 0);
    close(sock);

    return (void *)0; // É necessário cast para evitar warning
}

// Função que recebe as informações
void *data_receiver_thread(void *arg) {
    int sock;
    struct sockaddr_in me, from;
    socklen_t adl = sizeof(from); // Corrigido para 'from'
    char linha[ECHOMAX];

    // Cria o socket para enviar e receber datagramas parâmetros(familia, tipo, protocolo)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("ERRO na Criacao do Socket!\n");
    else
        printf("Servidor esperando mensagens...\n");

    // Construcao da estrutura do endereco local, preenchendo a estrutura socket me (familia, IP, porta)
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY); // Endereco IP local
    me.sin_port = htons(APP_PORT); // Porta local

    // Bind para o endereco local parametros(descritor socket, estrutura do endereco local, comprimento do endereco)
    if (-1 != bind(sock, (struct sockaddr *)&me, sizeof(me))) {
        do {
            // Bloqueio por mutex
            pthread_mutex_lock(&input_lock);
            while (pause_output) {
                pthread_cond_wait(&input_cond, &input_lock);
            }
            pthread_mutex_unlock(&input_lock);

            // Recebe mensagem do endereco remoto
            ssize_t recv_len = recvfrom(sock, linha, ECHOMAX - 1, 0, (struct sockaddr *)&from, &adl);
            if (recv_len > 0) {
                linha[recv_len] = '\0'; // Garante a terminação nula
                printf("Recebido:\n %s\n", linha);
            }
        } while (stop == 0);
    }
    else {
        puts("Porta ocupada");
    }

    // Fecha o socket
    close(sock);
    return 0;
}

// Controle de input do usuário
void *user_input_thread(void *arg) {
    char input;
    char in[100];

    do {
        printf("Aperte enter para usar input\n");
        if (fgets(in, sizeof(in), stdin) == NULL) {
            perror("fgets");
            continue;
        }

        pthread_mutex_lock(&input_lock);
        pause_output = 1;  // Pausa o fluxo das outras threads
        pthread_mutex_unlock(&input_lock);

        // Bloqueia a execução até o usuário apertar enter
        printf("1: Adicionar IP | 2: Alterar tempo de envio\nDigite um número:\n");
        input = getchar();
        while (getchar() != '\n'); // Limpa o buffer

        switch (input) {
            case '1':
                printf("Digite o IP a ser adicionado: ");
                if (fgets(in, sizeof(in), stdin) != NULL) {
                    // Remove o caractere de nova linha
                    in[strcspn(in, "\n")] = '\0';
                    add_ip_address(in);
                    printf("IP %s adicionado.\n", in);
                }
                break;
            case '2':
                // Chamar a função alter_interval
                alter_interval();
                break;
            default:
                printf("Opção inválida!\n");
        }

        pthread_mutex_lock(&input_lock);
        pause_output = 0;  // Sinaliza para as threads de retomarem
        pthread_cond_signal(&input_cond);
        pthread_mutex_unlock(&input_lock);
    } while (stop == 0);
    return (void*)0;
}

int main() {
    pthread_t threads[3];
    int thread_args[3]; 
    int i;
    char local_ip[100];

    // Busca e insere o IP local no array de IPs a enviar pacotes
    get_local_ip(local_ip, sizeof(local_ip));
    add_ip_address(local_ip);

    // Inicialização dos mutex
    pthread_mutex_init(&interval_lock, NULL);
    pthread_mutex_init(&input_lock, NULL);
    pthread_cond_init(&input_cond, NULL);

    // Criação das threads
    i = 0;
    thread_args[i] = i;
    if (pthread_create(&threads[i], NULL, data_receiver_thread, (void *)&thread_args[i]) != 0) {
        perror("pthread_create data_receiver_thread");
        exit(EXIT_FAILURE);
    }
    printf("Thread de recebimento de dados criada.\n");

    i = 1;
    thread_args[i] = i;
    if (pthread_create(&threads[i], NULL, data_sender_thread, (void *)&thread_args[i]) != 0) {
        perror("pthread_create data_sender_thread");
        exit(EXIT_FAILURE);
    }
    printf("Thread de envio criada.\n");

    i = 2;
    thread_args[i] = i;
    if (pthread_create(&threads[i], NULL, user_input_thread, (void *)&thread_args[i]) != 0) {
        perror("pthread_create user_input_thread");
        exit(EXIT_FAILURE);
    }
    printf("Thread de input criada\n");

    // Juntando as threads
    pthread_join(threads[0], NULL);
    pthread_join(threads[2], NULL);
    pthread_join(threads[1], NULL);

    printf("Programa finalizado!\n");

    // Destruir mutex e condição
    pthread_mutex_destroy(&interval_lock);
    pthread_mutex_destroy(&input_lock);
    pthread_cond_destroy(&input_cond);

    return 0;
}