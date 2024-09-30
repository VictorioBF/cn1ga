#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Função que será executada pelas threads
void *thread_function(void *arg) {
    int id = *((int *)arg);
    printf("Thread %d: Olá, mundo!\n", id);
    pthread_exit(NULL);
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
