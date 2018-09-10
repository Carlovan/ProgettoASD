#include "lib1718.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printQuery(query_t q) {
	const char* names[] = {"", "CREATE", "INSERT", "SELECT", "", "", "", "", "", "",
	                       "None", "WHERE", "GROUP BY", "ORDER BY", "", "", "", "", "", "", "",
	                       "==", ">", ">=", "<", "<=", "ASC", "DESC"};
	printf("Table name: %s\n", q.table);
	printf("Action: %s\n", names[q.action]);
	printf("Data:\n");
	for(query_data_t *d = q.data; d->colName != NULL || d->value != NULL; d++) {
		if(d->colName != NULL) {
			printf("\tColumn: %s\n", d->colName);
		}
		if(d->value != NULL) {
			printf("\tValue: %s\n", d->value);
		}
		printf("\n");
	}
	if(q.action == ACTION_SELECT && q.filter != FILTER_NONE) {
		printf("Filter: %s\n", names[q.filter]);
		printf("On the field: %s\n", q.filterField);
		if(q.filter != FILTER_GROUPBY) {
			printf("Operator: %s\n", names[q.op]);
		}
		if(q.filter == FILTER_WHERE) {
			printf("Value: %s\n", q.filterValue);
		}
	}
}

void testParsing() {
	char queryString[255];
	printf(">> Query: ");
	scanf("%[^\n]", queryString);
	while(strcmp(queryString, "q") != 0) {
		query_t parsed = newQuery();
		getchar();
		printf("%s\n\n", queryString);
		bool ok = parseQuery(queryString, &parsed);
		if(ok) {
			printQuery(parsed);
		} else {
			printf("MALE\n");
		}
		freeQuery(&parsed);
		printf(">> Query: ");
		scanf("%[^\n]", queryString);
	}
}

table_DB createTestTable() {
	table_DB t = newTable();
	t.table_name = (char*)malloc(13);
	strcpy(t.table_name, "nome_tabella");

	t.n_columns = 2;
	t.columns = (char**)malloc(t.n_columns * sizeof(char*));
	t.columns[0] = (char*)malloc(2);
	strcpy(t.columns[0], "a");
	t.columns[1] = (char*)malloc(2);
	strcpy(t.columns[1], "b");

	t.n_row = 2;
	t.data = (char***)malloc(t.n_row * sizeof(char**));
	for(size_t i = 0; i < t.n_row; i++) {
		t.data[i] = (char**)malloc(t.n_columns * sizeof(char*));
	}
	t.data[0][0] = (char*)malloc(6);
	t.data[0][1] = (char*)malloc(6);
	t.data[1][0] = (char*)malloc(6);
	t.data[1][1] = (char*)malloc(6);

	strcpy(t.data[0][0], "asd00");
	strcpy(t.data[0][1], "asd01");
	strcpy(t.data[1][0], "asd10");
	strcpy(t.data[1][1], "asd11");

	return t;
}

void testTableString() {
	table_DB t = createTestTable();

	char *tableStr = tableString(t);
	printf("Table:\n%s\n", tableStr);

	freeTable(&t);
	free(tableStr);
}

void testSaveTable() {
	table_DB t = createTestTable();

	saveTable(t);

	freeTable(&t);
}

void testLoadTable() {
	table_DB t = newTable();
	char name[] = "nome_tabella";

	loadTable(name, &t);
	printf("Table: %s\n", t.table_name);
	printf("Cols (%d):\n", t.n_columns);
	for(size_t i = 0; i < t.n_columns; i++) {
		printf("\t%s\n", t.columns[i]);
	}
	printf("Rows (%d):\n", t.n_row);
	for(size_t i = 0; i < t.n_row; i++) {
		putchar('\t');
		for(size_t j = 0; j < t.n_columns; j++) {
			printf("'%s' ", t.data[i][j]);
		}
		putchar('\n');
	}

	freeTable(&t);
}
