#ifndef LIB1718_H
#define LIB1718_H

#define bool int
#define true 1
#define false 0

#define MAX_S_LEN     256

#define ACTION_CREATE 1
#define ACTION_INSERT 2
#define ACTION_SELECT 3
typedef int action_t;

typedef struct {
	char colName[MAX_S_LEN];
	char value[MAX_S_LEN];
} query_data_t;

typedef struct {
	action_t action;
	char table[MAX_S_LEN];
	query_data_t *data;
} query_t;

/* Input:  La stringa contenente la query da eseguire, ad esempio "CREATE TABLE studenti (matricola, nome, cognome)"
 * Output: true/false, se l'esecuzione della query e' andata a buon fine o meno (presenza di eventuali errori)
 */
bool executeQuery(char*); 

bool parseQuery(char* query, query_t* parsed);

#endif //LIB1718_H
