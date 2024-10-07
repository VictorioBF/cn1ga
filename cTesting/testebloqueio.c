#include <stdio.h>
#include <pthread.h>
#include <unistd.h>  // For sleep()

pthread_mutex_t lock;
pthread_cond_t cond;
int pause_output = 0;

void* output_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        while (pause_output) {
            pthread_cond_wait(&cond, &lock);
            printf("I am blocked");
        }
        pthread_mutex_unlock(&lock);

        printf("Outputting to the console...\n");
        sleep(1);  // Simulate work
    }
    return NULL;
}

void* input_thread(void* arg) {
    char input[100];
    while (1) {
        fgets(input, sizeof(input), stdin);  // Wait for user to press Enter

        pthread_mutex_lock(&lock);
        pause_output = 1;  // Signal the output thread to pause
        pthread_mutex_unlock(&lock);

        printf("Press Enter again to resume...\n");
        fgets(input, sizeof(input), stdin);  // Wait for user to press Enter again

        pthread_mutex_lock(&lock);
        pause_output = 0;  // Signal the output thread to resume
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    // Initialize the mutex and condition variable
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // Create the threads
    pthread_create(&thread1, NULL, output_thread, NULL);
    pthread_create(&thread2, NULL, input_thread, NULL);

    // Wait for the threads to finish (they won't in this example)
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // Destroy the mutex and condition variable
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}
