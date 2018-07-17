#include "lib1718.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Chiama free su una lista di stringhe
void freeStrings(char*** l, size_t n) {
	for(int i = 0; i < n; i++)
		free((*l)[i]);
	free(*l);
}

// Libera tutta la memoria allocata per un query_t
void freeQuery(query_t *q) {
	if(q->table != NULL)
		free(q->table);
	if(q->data != NULL) {
		for(query_data_t *i = q->data; i->colName != NULL; i++) {
			free(i->colName);
			if(i->value != NULL)
				free(i->value);
		}
		free(q->data);
	}
}

// Restituisce un nuovo oggetto query_t
query_t newQuery() {
	query_t q = {0, NULL, NULL};
	return q;
}

// Restituisce un nuovo oggetto query_data_t
query_data_t newQueryData() {
	query_data_t q;
	q.colName = NULL;
	q.value = NULL;
	return q;
}

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
		for(; *c != 0 && *c != delim; c++) {
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
	// In questo caso l'ultimo output non è inizializzato
	if(s[strlen(s)-1] == delim) {
		*output = (char*)malloc(1);
		(*output)[0] = 0;
	}
	return count;
}

// Indica se la stringa passata è un nome di colonna valido.
//   - inizia con una lettera
//   - contiene solo lettere, numeri e underscore
bool isValidName(char* name) {
	if(name[0] == 0) return 0;
	if(!isalpha(name[0])) return 0;
	for(char *c = name; *c != 0; c++) {
		if(!isalpha(*c) && !isdigit(*c) && *c != '_')
			return 0;
	}
	return 1;
}

bool parseCreate(char* query, query_t* parsed) {
	parsed->action = ACTION_CREATE;
	int readCount = 0;
	char cols[1000], table[1000], semicolon[2];
	memset(cols, 0, 1000);
	memset(table, 0, 1000);
	memset(semicolon, 0, 2);

	readCount = sscanf(query, " CREATE TABLE %s (%[^)]) %1[;]", table, cols, semicolon);
	if(readCount != 3) return 0;
	parsed->table = (char*)malloc(strlen(table) + 1);
	strcpy(parsed->table, table);

	char **colNames;
	size_t colCount = splitAndTrim(cols, ',', &colNames);

	for(int i = 0; i < colCount; i++) {
		if(!isValidName(colNames[i])) {
			freeStrings(&colNames, colCount);
			return 0;
		}
	}

	parsed->data = (query_data_t*)malloc((colCount+1) * sizeof(query_data_t));
	for(size_t i = 0; i < colCount; i++) {
		parsed->data[i] = newQueryData();
		parsed->data[i].colName = colNames[i];
	}
	parsed->data[colCount] = newQueryData(); // Inizializzo anche l'ultimo che è come terminatore

	free(colNames); // Libero solo la lista, le stringhe sono ancora utilizzate
	return 1;
}

// Indica se la stringa passata è un valore valido
//   - inizia e finisce con ' (stringa)
//   - non inizia con zero E sono tutte cifre (numero)
bool isValidValue(char *val) {
	size_t valLen = strlen(val);
	if(valLen == 0)
		return 0;
	// È una stringa
	if(valLen >= 2 && (val[0] == '\'' && val[valLen-1] == '\''))
		return 1;
	// Controllo se è un numero valido
	if(val[0] == '0') // Non puo iniziare per 0
		return 0;
	for(char *c = val; *c != 0; c++) {
		if(!isdigit(*c))
			return 0;
	}
	return 1;
}

bool parseInsert(char* query, query_t* parsed) {
	parsed->action = ACTION_INSERT;
	int readCount = 0;
	char cols[1000], table[1000], values[1000], semicolon[2];
	memset(cols, 0, 1000);
	memset(table, 0, 1000);
	memset(values, 0, 1000);
	memset(semicolon, 0, 2);

	readCount = sscanf(query, " INSERT INTO %s (%[^)]) VALUES (%[^)]) %1[;]", table, cols, values, semicolon);
	if(readCount != 4) return 0;
	parsed->table = (char*)malloc(strlen(table) + 1);
	strcpy(parsed->table, table);

	char **colNames;
	size_t colCount = splitAndTrim(cols, ',', &colNames);
	for(size_t i = 0; i < colCount; i++) {
		if(!isValidName(colNames[i])) {
			freeStrings(&colNames, colCount);
			return 0;
		}
	}

	char **valList;
	size_t valCount = splitAndTrim(values, ',', &valList);
	for(size_t i = 0; i < valCount; i++) {
		if(!isValidValue(valList[i])) {
			freeStrings(&colNames, colCount);
			freeStrings(&valList, valCount);
			return 0;
		}
	}

	if(valCount != colCount) {
		freeStrings(&colNames, colCount);
		freeStrings(&valList, valCount);
		return 0;
	}

	parsed->data = (query_data_t*)malloc((colCount+1) * sizeof(query_data_t));
	for(size_t i = 0; i < colCount; i++) {
		parsed->data[i] = newQueryData();
		parsed->data[i].colName = colNames[i];
		parsed->data[i].value = valList[i];
	}
	parsed->data[colCount] = newQueryData(); // Inizializzo anche l'ultimo che è come terminatore

	free(colNames); // Libero solo la lista, le stringhe sono ancora utilizzate
	free(valList);
	return 1;
}

bool parseSelect(char *query, query_t* parsed) {
	parsed->action = ACTION_SELECT;
	char cols[1000], table[1000], semicolon[2];
	memset(cols, 0, 1000);
	memset(table, 0, 1000);
	memset(semicolon, 0, 2);

	// Trovo la prima occorrenza di 'SELECT' e controllo che sia preceduta solo da spazi
	char *SELECT = strstr(query, "SELECT");
	if(SELECT == NULL)
		return 0;
	for(char *c = query; c < SELECT; c++) {
		if(!isblank(*c))
			return 0;
	}

	// Trovo la prima occorrenza di FROM: quello che è tra SELECT e FROM è la lista delle colonne
	char *FROM = strstr(query, "FROM");
	if(FROM == NULL)
		return 0;
	strncpy(cols, SELECT+6, FROM-(SELECT+6));

	// Splitto la lista di colonne e controllo che siano nomi validi
	char **colNames;
	size_t colCount = splitAndTrim(cols, ',', &colNames);
	for(size_t i = 0; i < colCount; i++) {
		if(!isValidName(colNames[i])) {
			freeStrings(&colNames, colCount);
			return 0;
		}
	}

	parsed->data = (query_data_t*)malloc((colCount+1) * sizeof(query_data_t));
	for(size_t i = 0; i < colCount; i++) {
		parsed->data[i] = newQueryData();
		parsed->data[i].colName = colNames[i];
	}
	parsed->data[colCount] = newQueryData();
	free(colNames); // Libero solo il puntatore, le stringhe sono in parsed->data

	// Prendo il nome della tabella
	char *c;
	for(c = FROM+4; *c != 0 && isblank(*c); c++);
	for(size_t i = 0; *c != 0 && !isblank(*c); i++, c++) {
		table[i] = *c;
	}
	if(!isValidName(table))
		return 0;


	char *end = strstr(FROM, ";");
	if(end == NULL) {
		return 0;
	}

	return 1;
}

bool parseQuery(char* query, query_t* parsed) {
	int readCount;

	char command[1000];
	readCount = sscanf(query, " %s ", command);
	if(readCount != 1) return 0;

	if(strcmp("CREATE", command) == 0) {
		return parseCreate(query, parsed);
	} else if(strcmp("INSERT", command) == 0) {
		return parseInsert(query, parsed);
	} else if(strcmp("SELECT", command) == 0) {
		return parseSelect(query, parsed);
	} else {
		parsed->data = (query_data_t*)malloc(sizeof(query_data_t));
		parsed->data[0] = newQueryData();
	}

	return 0;
}
