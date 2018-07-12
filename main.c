#include "lib1718.h"
#include <stdio.h>

int main() {
	query_t parsed = newQuery();
	bool ok = parseQuery("    INSERT  INTO giovanni (i,	 ciccione_8   ) VALUES (1, '123   '   );", &parsed);
	if(ok) {
		printf("%d, %s\nCOLS:\n", parsed.action, parsed.table);
		for(query_data_t *i = parsed.data; i->colName != NULL; i++)
			printf("%s %s\n", i->colName, i->value);
	} else {
		printf("MALE\n");
	}
	freeQuery(&parsed);
}
