#ifndef LIB1718_H
#define LIB1718_H

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

	// Lista di colonne/valori interessate dalla query, terminata da un valore con colName e value entrambi NULL:
	// colonne tabella per CREATE
	// colonne e valori da inserire per INSERT
	// colonne da visualizzare per SELECT
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
	int n_columns;
	int n_row;
} table_DB;






/* Input:  La stringa contenente la query da eseguire, ad esempio "CREATE TABLE studenti (matricola, nome, cognome)"
 * Output: true/false, se l'esecuzione della query e' andata a buon fine o meno (presenza di eventuali errori)
 */
bool executeQuery(char*str); 

bool parseQuery(char* query, query_t* parsed); //create insert select
bool parseSelect(char *query, query_t* parsed);
bool parseInsert(char* query, query_t* parsed);
bool parseCreate(char* query, query_t* parsed);
bool isValidValue(char *val); //da rivedere
bool isValidName(char* name); //perchè non puoi inizare per numero e non può avere underscore
void freeQuery(query_t*);//forse da rivedere
query_t newQuery();//forse da rifare
query_data_t newQueryData();//forse da rifare 

//blocco ordinamento di una tabella per una determinata colonna
bool sortDB(table_DB* DB, char *columns);//funzione master per l'ordinamento di una tabella; ordina la tabella per la colonna data 
bool sortDBnum(table_DB* DB,int id_columns);//funzione slave: ordina per numeri
void sortDBstrQUICKSORT(table_DB* DB,int id_columns, int low, int high);//funzione slave: ordina per stringa
int srcCOLUMNS(char**columns, char* src,int n_columns);//ritorna l'indice della colonna da ordinare es: columns={nome,cognome,tel} src={tel} return=2; return=-1 se non esiste
bool identifyINT(char* elem);//ritorna false se elem è stringa; ritorna true se elem è un numero
void sortDBnumQUICKSORT(table_DB*DB, int vet[], int low, int high);//ordina per una colonna di interi una tabella
int sortDBnumPARTITION(table_DB*DB, int vet[], int low, int high);//partition della funzione sortDBnumQUICKSORT
void sortDBstrQUICKSORT(table_DB* DB, int id_columns, int low, int high);//ordiamento di stringhe
int sortDBstrPARTITION(table_DB*DB, int id_columns, int low, int high);//partition della funzione sortDBnumQUICKSORT
void sortDBnumSWAP(int* a, int* b,char***c,char***d);
void sortDBstrSWAP(char***a, char***b);

/********select********/
bool selectORDERby(char*order_by, bool desc, table_DB*DB);//ordina la tabella pre una data colonna, DESC vale TRUE solo se devo ordinare in maniera DECRESCENETE
int* selectGROUPby(char*group_by, table_DB*DB);


#endif //LIB1718_H
