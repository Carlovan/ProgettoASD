#ifndef LIB1718_H
#define LIB1718_H
typedef unsigned int     size_t;
#define bool int
#define true 1
#define false 0

#define ACTION_CREATE 1
#define ACTION_INSERT 2
#define ACTION_SELECT 3
typedef int action_t;


#define FILTER_NONE 10
#define FILTER_WHERE 11
#define FILTER_GROUPBY 12
#define FILTER_ORDERBY 13
typedef int filter_t;

#define OP_EQ 20 // Uguale
#define OP_GT 21 // Maggiore
#define OP_GE 22 // Maggiore o uguale
#define OP_LT 23 // Minore
#define OP_LE 24 // Minore o uguale
#define OP_ASC 25 // Ordine ascendente
#define OP_DESC 26 // Ordine discendente
typedef int filter_op_t;

// Rappresenta una coppia colonna valore, usata nell'interpretazione della query
typedef struct { 
	char *colName;
	char *value;
} query_data_t;

// Rappresenta le caratteristiche di una query
typedef struct {
	char *table; // Nome tabella
	action_t action; // SELECTION, CREATE, INSERT
	query_data_t *data;

	// Specifici per SELECT
	filter_t filter; // NONE, WHERE, GROUP, ORDER
	filter_op_t op; // Operatore per WHERE, ORDER
	char *filterField; // Campo su cui applicare il filtro
	char *filterValue; // Valore con cui confrontare il campo
} query_t;

// Rappresenta una tabella
typedef struct {
	char* table_name;
	char** columns;
	char*** data; // Dati nella tabella. es: data[riga][colonna]
	size_t n_columns;
	size_t n_row;
} table_DB;






/* Input:  La stringa contenente la query da eseguire, ad esempio "CREATE TABLE studenti (matricola, nome, cognome)"
 * Output: true/false, se l'esecuzione della query e' andata a buon fine o meno (presenza di eventuali errori)
 */
bool executeQuery(char*str); 

bool parseQuery(char* query, query_t* parsed);
void freeQuery(query_t*);
query_t newQuery(void);
query_data_t newQueryData(void);
table_DB newTable(void);
void freeTable(table_DB* t);

char* tableString(table_DB table);
void saveTable(table_DB table);
bool loadTable(char* name, table_DB* table);

#endif //LIB1718_H
