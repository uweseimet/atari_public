/*********************************//* SCSI2Pi Command Executor 1.00 *//*                               */
/* (C) 2023 Uwe Seimet           *//*********************************/

#define VERSION "1.00"
#include <stdio.h>#include <stdlib.h>#include <std.h>#include <string.h>#include "jsmn/jsmn.h"
#include "pi_lib.h"


const char *BANNER[] = {
	"\n\x1b\x70SCSI2Pi command client V" VERSION "\x1b\x71",	"\n\x1b\x70SCSI2Pi Command-Client V" VERSION "\x1b\x71"};

const char *USAGE[] = {
	"Usage: PI_EXEC [-b|-j|-t input file] [-B|-J|-T output file] [-m]\n",
	"Nutzung: PI_EXEC [-b|-j|-t EingabeDatei] [-B|-J|-T Ausgabedatei] [-m]\n"
};

const char *ERROR_OPEN_INPUT[] = {
	"Could not open input file",
	"Eingabedatei konnte nicht geîffnet werden"
};

const char *ERROR_OPEN_OUTPUT[] = {
	"Could not open output file",
	"Ausgabedatei konnte nicht geîffnet werden"
};

const char *ERROR_TRANSFER_LENGTH[] = {
	"Input file is bigger than 64 K",
	"Eingabedatei ist grîûer als 64 K"
};

const char *ERROR_MEMORY[] = {
	"Insufficient main memory",
	"Nicht genÅgend Speicher"
};

const char *ERROR_JSON[] = {
	"Could not parse JSON data",
	"JSON-Daten konnten nicht geparsed werden"
};


const char *SAMPLE_INPUT = "{\"operation\":\"SERVER_INFO\"}";


int inputFormat = json;
int outputFormat = json;

const char *inputFile;
const char *outputFile;

FILE *in;
FILE *out;

const char *input;

char results[MAX_TRANSFER_LENGTH];


bool displayResults(const char *);


intmain(int argc, char *argv[]){
	/* In test mode use the input data as output data */
	bool testMode = false;
	int i;

	setLanguage();

	printLocalized(BANNER);
	printf("Ω 2023 Uwe Seimet\n");

	/* By default execute a sample command */
	input = SAMPLE_INPUT;

	if (argc == 2) {
		input = argv[1];
	}

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'm') {
				testMode = true;
				continue;
			}

			if (i + 1 >= argc) {
				break;
			}

			switch (argv[i++][1]) {
			case 'b':
				inputFormat = binary;
				inputFile = argv[i];
				break;

			case 'j':
				inputFormat = json;
				inputFile = argv[i];
				break;

			case 't':
				inputFormat = text;
				inputFile = argv[i];
				break;

			case 'B':
				outputFormat = binary;
				outputFile = argv[i];
				break;

			case 'J':
				outputFormat = json;
				outputFile = argv[i];
				break;

			case 'T':
				outputFormat = text;
				outputFile = argv[i];
				break;

			default:
				printLocalized(USAGE);
				return -1;
			}
		}
	}

	if (inputFile) {
		in = fopen(inputFile, inputFormat == binary ? "rb" : "r");
		if (!in) {
			return printLocalized(ERROR_OPEN_INPUT);
		}
	}

	if (outputFile) {
		out = fopen(outputFile, outputFormat == binary ? "wb" : "w");
		if (!out) {
			fclose(in);
			return printLocalized(ERROR_OPEN_OUTPUT);
		}
	}

	if (in) {
		size_t length;

		fseek(in, 0, SEEK_END);
		length = ftell(in);

		if (length > MAX_TRANSFER_LENGTH) {
			fclose(in);
			fclose(out);
			return printLocalized(ERROR_TRANSFER_LENGTH);
		}

		fseek(in, 0, SEEK_SET);

		input = malloc(length);
		if (!input)  {
			fclose(in);
			fclose(out);
			return printLocalized(ERROR_MEMORY);
		}

		fread(input, length, 1, in);
		fclose(in);
		free(input);
	}

	if (!testMode && in && out) {
		if (!executeCommand, inputFormat, input, strlen(input),
			outputFormat, results, sizeof(results)) {
			fclose(out);
			free(input);
			return -1;
		}
	}
	else {
		strcpy(results, input);
	}

	free(input);

	if (out) {
		fprintf(out, "%s\n", results);
		fclose(out);
	}
	else if (outputFormat == text) {
		printf("%s\n", results);
	}

	if (outputFormat == json) {
		if (!displayResults(results)) {
			return printLocalized(ERROR_JSON);
		}
	}
	
	return 0;		
}


bool
displayResults(const char *output)
{
	int i;
	jsmntok_t *tokens;

	const int tokenCount = parseJson(output, &tokens);
	if (tokenCount < 0) {
		return false;
	}

	for (i = 0; i < tokenCount; i++) {
		int length;
		char *value;

		printf("Type: %d\n", tokens[i].type);

		length = tokens[i].end - tokens[i].start;
		value = malloc(length + 1);
		strncpy(value, output + tokens[i].start, length);
		value[length] = 0;
		printf("Value: %s\n", value);
		free(value);

		printf("Children: %d\n", tokens[i].size);
	}

	free(tokens);

	return true;
}
