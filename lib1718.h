#ifndef LIB1718_H
#define LIB1718_H

#define bool int
#define true 1
#define false 0

#define ACTION_CREATE 1
#define ACTION_INSERT 2
#define ACTION_SELECT 3
typedef int action_t;

#define FILTER_WHERE 10
#define FILTER_GROUPBY 11
#define FILTER_ORDERBY 12
typedef int filter_t;

#define OP_EQ 20
#define OP_GT 21
#define OP_GE 22
#define OP_LT 23
#define OP_LE 24
#define OP_ASC 25
#define OP_DESC 26
typedef int filter_op_t;

typedef struct {
	char *colName;
	char *value;
} query_data_t;

typedef struct {
	action_t action;
	char *table;
	query_data_t *data;
	filter_t filter;
	char *filterField;
	filter_op_t op;
	char *filterValue;
} query_t;

/* Input:  La stringa contenente la query da eseguire, ad esempio "CREATE TABLE studenti (matricola, nome, cognome)"
 * Output: true/false, se l'esecuzione della query e' andata a buon fine o meno (presenza di eventuali errori)
 */
bool executeQuery(char*); 

bool parseQuery(char* query, query_t* parsed);
void freeQuery(query_t*);
query_t newQuery();
query_data_t newQueryData();
#endif //LIB1718_H
