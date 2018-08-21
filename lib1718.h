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
	char *table;// non dovrebbe essere un char***?	%	
	query_data_t *data;
	filter_t filter;
	char *filterField;
	filter_op_t op;
	char *filterValue;
} query_t;

//stuttura per il caricamento della tabella
typedef struct {
	char* table_name;//nome della tabella
	char** columns;//nume delle colonne
	char*** data;//memorizazione di tutta la tabella

}table_DB;





/* Input:  La stringa contenente la query da eseguire, ad esempio "CREATE TABLE studenti (matricola, nome, cognome)"
 * Output: true/false, se l'esecuzione della query e' andata a buon fine o meno (presenza di eventuali errori)
 */
bool executeQuery(char*); 

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
int srcCOLUMNS(char**columns, char* src);//ritorna l'indice della colonna da ordinare es: columns={nome,cognome,tel} src={tel} return=2; return=-1 se non esiste
bool identifyINT(char* elem);//ritorna false se elem è stringa; ritorna true se elem è un numero
void sortDBnumQUICKSORT(table_DB*DB, int vet[], int low, int high);//ordina per una colonna di interi una tabella
int sortDBnumPARTITION(table_DB*DB, int vet[], int low, int high);//partition della funzione sortDBnumQUICKSORT
void sortDBnumSWAP(int* a, int* b,char***c,char***d);
void sortDBstrSWAP(char***a, char***b);

/********select********/
bool select_order_by(char*order_by, bool desc, table_DB*DB);//ordina la tabella pre una data colonna, DESC vale TRUE solo se devo ordinare in maniera DECRESCENETE



#endif //LIB1718_H
