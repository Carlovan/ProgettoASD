#include "lib1718.h"
#include <stdio.h>

int main() {
	query_t parsed = newQuery();
	bool ok = parseQuery("    CREATE TABLE giovanni (i,	 ciccione_8   ) ;", &parsed);
	if(ok) {
		printf("%d, %s\nCOLS:\n", parsed.action, parsed.table);
		for(query_data_t *i = parsed.data; i->colName != NULL; i++)
			printf("%s\n", i->colName);
	} else {
		printf("MALE\n");
	}
	freeQuery(&parsed);
}
