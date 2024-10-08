#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp;
    char path[1035];

    // Executa o comando vmstat e abre um pipe para leitura
    fp = popen("vmstat", "r");
    if (fp == NULL) {
        printf("Falha ao executar o comando vmstat\n");
        return 1;
    }

    // Lê a saída do comando linha por linha e imprime na tela
    // printf(i);
    while (fgets(path, sizeof(path), fp) != NULL) {
        printf("%s", path);
    }

    // Fecha o pipe
    pclose(fp);

    return 0;
}
