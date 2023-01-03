#include "Header_File/headers.h"
#include "time.h"

int main(int agrc, char *argv[]) {

    int runtime = atoi(argv[1]);
    while ((clock() / CLOCKS_PER_SEC) < runtime);

    exit(EXIT_SUCCESS);
}