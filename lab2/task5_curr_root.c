#include <stdio.h>
#include <dirent.h>

#define EXIT_ON_ERROR -1

int printDirectory(const char *path) {
    DIR *directory = opendir(path);
    if (!directory) {
        fprintf(stderr, "Error when opening the specified directory: %s\n", path);
        return EXIT_ON_ERROR;
    }

    struct dirent *dirOrFile;

    while ((dirOrFile = readdir(directory))) {
        printf("%s\n", dirOrFile->d_name);
    }

    if (closedir(directory)) {
        fprintf(stderr, "Error when closing the specified directory: %s\n", path);
        return EXIT_ON_ERROR;
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    int errCode = 0;
    printf("Current directory:\n");
    errCode = printDirectory("./");
    if (errCode) return errCode;
    printf("\nRoot directory:\n");
    errCode = printDirectory("/");
    if (errCode) return errCode;
    return errCode;
}