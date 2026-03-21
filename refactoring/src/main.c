#include <stdio.h>

__attribute__((weak))
int app_main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("[ERROR] Nenhuma app selecionada na compilação\n");
    return -1;
}

int main(int argc, char *argv[]) {
    app_main(argc, argv);
}