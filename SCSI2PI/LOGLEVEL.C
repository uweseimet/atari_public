/*******************************//* SCSI2Pi Log Level Tool 1.00 *//*                             */
/* (C) 2023 Uwe Seimet         */
/*******************************/

#define VERSION "1.00"
#include <stdio.h>#include <stdlib.h>#include <std.h>#include <string.h>#include "jsmn/jsmn.h"
#include "pi_lib.h"


const char *BANNER[] = {
	"\n\x1b\x70SCSI2Pi loglevel tool V" VERSION "\x1b\x71",	"\n\x1b\x70SCSI2Pi Loglevel-Tool V" VERSION "\x1b\x71"};


const char *LOG_LEVEL_INFO = "{\"operation\":\"LOG_LEVEL_INFO\"}";
const char *SET_LOG_LEVEL = "{\"operation\":\"SET_LOG_LEVEL\"}";

const char *PARAMS1 = "{\"operation\":\"LOG_LEVEL\",\"params\":{\"level\":\"";
const char *PARAMS2 ="\"}}";

char results[MAX_TRANSFER_LENGTH];


bool setLevel(const char *);


int
test()
{
	size_t length;
	const char *input;

	FILE *in = fopen("test.json", "r");
	if (!in) {
		return -1;
	}


	fseek(in, 0, SEEK_END);
	length = ftell(in);

	if (length > MAX_TRANSFER_LENGTH) {
		fclose(in);
		return -1;
	}

	fseek(in, 0, SEEK_SET);

	input = malloc(length);
	if (!input)  {
		fclose(in);
		return -1;
	}

	fread(input, length, 1, in);
	fclose(in);
	free(input);

	setLevel(input);

	return 0;
}


intmain()
{
	int tokenCount;
	jsmntok_t *tokens;

	setLanguage();

	printLocalized(BANNER);
	printf("½ 2023 Uwe Seimet\n");

/*
	if (!test()) return -1;
	return 0;
*/

	if (!executeCommand(json, LOG_LEVEL_INFO, (int)strlen(LOG_LEVEL_INFO),
		json, results, (int)sizeof(results))) {
		return -1;
	}

	tokenCount = parseJson(results, &tokens);
	if (tokenCount < 0) {
		return -1;
	}

	if (!setLevel(results)) {
		return -1;
	}
	
	return 0;		
}


bool
setLevel(const char *output)
{
	jsmntok_t *tokens;
	char operation[100];
	char newLogLevel[10];
	int index = 0;

	const int tokenCount = parseJson(output, &tokens);
	if (tokenCount < 0) {
		return false;
	}

	index = findJsonNode(output, tokens, tokenCount, "currentLogLevel");
	if (!index) {
		free(tokens);
		return false;
	}
	else {
		const char *currentLogLevel = getJsonString(output, &tokens[index]);
		printf("\nCurrent log level: %s\n\n", currentLogLevel);
		free(currentLogLevel);
	}

	index = findJsonNode(output, tokens, tokenCount, "logLevels");
	if (!index) {
		free(tokens);
		return false;
	}
	else {
		int j;
		const jsmntok_t *token = &tokens[index];

		printf("Select the new log level from the available levels:\n");

		for (j = 1; j <= token->size; j++) {
			const char *logLevel = getJsonString(output, &tokens[++index]);
			printf("  %s\n", logLevel);
			free(logLevel);
		}
	}

	free(tokens);

	printf("\n");

	if (scanf("%9s", newLogLevel) == 1) {
		printf("\nSetting log level to %s\n", newLogLevel);
	}

	sprintf(operation, "%s%s%s", PARAMS1, newLogLevel, PARAMS2);

	if (!executeCommand(json, operation, (int)strlen(operation),
		json, results, (int)sizeof(results))) {
		return false;
	}

	return true;
}
