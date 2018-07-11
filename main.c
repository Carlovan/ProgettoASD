#include "lib1718.h"
#include <stdio.h>

int main() {
	query_t parsed;
	bool ok = parseQuery("    CREATE TABLE giovanni (     ,,,	 ciccione 8   ) ;", &parsed);
	if(ok) {
		printf("%d, %s\n", parsed.action, parsed.table);
	} else {
		printf("MALE\n");
	}
}
