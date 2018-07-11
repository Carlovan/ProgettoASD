#include "lib1718.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char trash[1000];

/* Divide la stringa s usando delim come delimitatore.
 * Rimuove gli spazi all'inizio e alla fine di ogni pezzo.
 * Alloca splits e ci mette tutti i pezzi.
 */
size_t splitAndTrim(char* s, const char delim, char*** splits) {
	// Conto quanti pezzi ci sono, considerando delimitatori in fila come uno unico
	size_t count = 1;
	for(char *c = s; *c != 0; c++) {
		if(*c == delim)
			count++;
	}
	
	// Alloco la memoria necessaria
	*splits = (char**)malloc(count * sizeof(char*));
	char** output = *splits;

	// Scorro tutta la stringa
	for(char *c = s; *c != 0; c++) {
		char *tb = 0, *te = 0;
		for(;*c != 0 && *c != delim; c++) {
			if(!isblank(*c)) {
				if(tb == NULL)
					tb = c;
				te = c;
			}
		}
		size_t len = (te==0 ? 0 : te-tb+1);
		*output = (char*)malloc(len + 1);
		memcpy(*output, tb, len);
		(*output)[len] = 0;
		output++;
	}
	return count;
}

bool parseCreate(char* query, query_t* parsed) {
	parsed->action = ACTION_CREATE;
	int readCount = 0;
	char cols[1000];

	readCount = sscanf(query, " CREATE TABLE %s (%[^)]) %1[;]", parsed->table, cols, trash);
	if(readCount != 3) return 0;

	char **colNames;
	size_t colCount = splitAndTrim(cols, ',', &colNames);
	printf("%lu\n", colCount);
	for(int i = 0; i < colCount; i++)
		printf("%s\n", colNames[i]);

	// Libero la memoria
	for(int i = 0; i < colCount; i++)
		free(colNames[i]);
	free(colNames);

	return 1;
}

bool parseQuery(char* query, query_t* parsed) {
	int readCount;
	
	char command[MAX_S_LEN];
	readCount = sscanf(query, " %s ", command);
	if(readCount != 1) return 0;

	if(strcmp("CREATE", command) == 0) {
		return parseCreate(query, parsed);
	} else if(strcmp("INSERT", command) == 0) {
		parsed->action = ACTION_INSERT;
	} else if(strcmp("SELECT", command) == 0){
		parsed->action = ACTION_SELECT;
	}

	return 0;
}
