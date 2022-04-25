#include <stdio.h>

#define EXIT_ON_ERROR -1

int main(int argc, char *argv[]) {
	if (argc < 2){
		fprintf(stderr,"Wrong argument selection.\nProper format: %s <filename>\n", argv[0]);
		return EXIT_ON_ERROR;
	}
	FILE* file = fopen(argv[1], "w");

	if (file == NULL){
		fprintf(stderr,"File %s couldn't have been written to\n", argv[1]);
		return EXIT_ON_ERROR;
	}

	char nextChar;
	while ((nextChar = getc(stdin)) != '\006'){
		fputc(nextChar, file);
	}

	if(fclose(file)){
		fprintf(stderr,"Error when closing\n", argv[1]);
		return EXIT_ON_ERROR;
	}
	return 0;
}
