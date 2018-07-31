#include "lib1718.h"
#include <stdio.h>

int main() {
	query_t parsed = newQuery();
	bool ok = parseQuery("    SELECT  i, a, b FROM asd ORDER asd ;", &parsed);
	if(ok) {
		printf("%d, %s, %d\nCOLS:\n", parsed.action, parsed.table, parsed.filter);
		for(query_data_t *i = parsed.data; i->colName != NULL; i++)
			printf("%s %s\n", i->colName, i->value);
	} else {
		printf("MALE\n");
	}
	freeQuery(&parsed);
}
