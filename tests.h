#include "lib1718.h"
#include <stdio.h>
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
