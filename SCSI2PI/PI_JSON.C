/*******************************//* SCSI2Pi client library 3.00 *//*                             *//* (C) 2022-2023 Uwe Seimet    *//*******************************/
#define JSMN_STATIC
#include <stdio.h>
#include <stdlib.h>#include <string.h>
#include "pi_lib.h"


static const char *ERROR_JSON[] = {
	"Could not process JSON data",
	"JSON-Daten konnten nicht verarbeitet werden"
};


int
parseJson(const char *json, jsmntok_t **tokens)
{
	int tokenCount;
	jsmn_parser parser;
	
	jsmn_init(&parser);
	tokenCount = jsmn_parse(&parser, json, strlen(json), NULL, 0);
	if (tokenCount <= 0) {
		return printLocalized(ERROR_JSON);
	}

	*tokens = malloc(tokenCount * sizeof(jsmntok_t));
	if (!tokens) {
		return printLocalized(ERROR_JSON);
	}

	jsmn_init(&parser);
	jsmn_parse(&parser, json, strlen(json), *tokens, tokenCount);

	return tokenCount;
}



int
findJsonNode(const char *data, jsmntok_t *tokens, int tokenCount,
	const char *node)
{
	int i = 1;
	while (i < tokenCount - 1) {
		const jsmntok_t *token = &tokens[i++];

		if (!strncmp(node, data + token->start, token->end - token->start)) {
			return i;
		}
	}

	return 0;
}


char
*getJsonString(const char *data, jsmntok_t *token)
{
	char *value;

	const int length = token->end - token->start;
	value = malloc(length + 1);
	strncpy(value, data + token->start, length);
	value[length] = 0;
	return value;
}

