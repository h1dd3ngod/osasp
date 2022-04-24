#include <stdio.h>

int main(int argc, char *argv[]) {
	if (argc < 2){
		fprintf(stderr,"Wrong argument selection.\nProper format: %s <filename>\n", argv[0]);
		return 1;
	}
	FILE* file = fopen(argv[1], "w");

	if (file == NULL){
		fprintf(stderr,"File %s couldn't have been written to\n", argv[1]);
		return 1;
	}

	char nextChar;
	while ((nextChar = getc(stdin)) != '\006'){
		fputc(nextChar, file);
	}

	if(fclose(file)){
		fprintf(stderr,"Error when closing\n", argv[1]);
		return 1;
	}
	return 0;
}
