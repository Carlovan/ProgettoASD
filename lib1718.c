#include "lib1718.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>


// Chiama free su una lista di stringhe
void freeStrings(char*** l, size_t n) {
	if(*l != NULL) {
		for(int i = 0; i < n; i++)
			if((*l)[i] != NULL)
				free((*l)[i]);
		free(*l);
	}
}

// Libera tutta la memoria allocata per un query_t
void freeQuery(query_t *q) {
	if(q->table != NULL)
		free(q->table);
	if(q->filterField != NULL)
		free(q->filterField);
	if(q->filterValue != NULL)
		free(q->filterValue);
	if(q->data != NULL) {
		for(query_data_t *i = q->data; i->colName != NULL || i->value != NULL; i++) {
			if(i->colName != NULL)
				free(i->colName);
			if(i->value != NULL)
				free(i->value);
		}
		free(q->data);
	}
}

// Libera tutta la memoria allocata per un table_DB
void freeTable(table_DB* t) {
	if(t->table_name != NULL) {
		free(t->table_name);
	}

	if(t->columns != NULL) {
		freeStrings(&t->columns, t->n_columns);
	}

	if(t->data != NULL) {
		for(size_t i = 0; i < t->n_row; i++) {
			freeStrings(&t->data[i], t->n_columns);
		}
		free(t->data);
	}
}

// Restituisce un nuovo oggetto query_t
query_t newQuery() {
	query_t q;
	q.table = NULL;
	q.action = 0;
	q.data = NULL;
	q.filter = FILTER_NONE;
	q.op = 0;
	q.filterField = NULL;
	q.filterValue = NULL;
	return q;
}

// Restituisce un nuovo oggetto query_data_t
query_data_t newQueryData() {
	query_data_t q;
	q.colName = NULL;
	q.value = NULL;
	return q;
}

table_DB newTable() {
	table_DB t;
	t.table_name = NULL;
	t.columns = NULL;
	t.data = NULL;
	t.n_columns = 0;
	t.n_row = 0;
	return t;
}

/* Divide la stringa s usando delim come delimitatore.
 * Rimuove gli spazi all'inizio e alla fine di ogni pezzo.
 * Alloca splits e ci mette tutti i pezzi.
 */
size_t splitAndTrim(char* s, const char delim, char*** splits) {
	// Conto quanti pezzi ci sono
	size_t count = 1;
	for(char *c = s; *c != 0; c++) {
		if(*c == delim)
			count++;
	}

	// Alloco la memoria necessaria
	*splits = (char**)malloc(count * sizeof(char*));
	char** output = *splits;

	// Scorro tutta la stringa
	for(char *c = s; *c != 0; c++) {
		char *tb = 0, *te = 0;
		for(; *c != 0 && *c != delim; c++) {
			if(!isblank(*c)) {
				if(tb == NULL)
					tb = c;
				te = c;
			}
		}
		size_t len = (te==0 ? 0 : te-tb+1);
		*output = (char*)malloc(len + 1);
		memcpy(*output, tb, len);
		(*output)[len] = 0;
		output++;
		if(*c == 0) break;
	}
	// In questo caso l'ultimo output non è inizializzato
	if(s[strlen(s)-1] == delim) {
		*output = (char*)malloc(1);
		(*output)[0] = 0;
	}
	return count;
}

// Indica se la stringa passata è un nome di colonna valido.
//   - inizia con una lettera
//   - contiene solo lettere, numeri e underscore
bool isValidName(char* name) {
	if(name[0] == 0) return 0;
	if(!isalpha(name[0])) return 0;
	for(char *c = name; *c != 0; c++) {
		if(!isalpha(*c) && !isdigit(*c) && *c != '_')
			return 0;
	}
	return 1;
}

// Estrae una singola parola da una stringa (gruppo di non blank delimitato da blank) e rimuove i blank
// Salta tutti i blank all'inizio della stringa e inizia a leggere dal primo non blank.
// Si ferma quando trova un altro blank, alloca la memoria necessaria per dest e copia la stringa.
// Se ci sono solo blank dest = NULL
// Restituisce il primo byte non letto e non scartato
char* readTrimWord(char* str, char** dest, char terminator) {
	*dest = NULL;
	for(; *str != terminator && isblank(*str); str++);
	if(*str == terminator) return str;
	char *c;
	for(c = str; *c != 0 && *c != terminator && !isblank(*c); c++);
	size_t len = c - str;
	*dest = (char*)malloc(len + 1);
	strncpy(*dest, str, len);
	(*dest)[len] = 0;
	return c;
}

// Indica se la stringa passata è un valore valido
//   - inizia e finisce con ' (stringa)
//   - non inizia con zero E sono tutte cifre (numero)
bool isValidValue(char *val) {
	size_t valLen = strlen(val);
	if(valLen == 0)
		return 0;
	// È una stringa
	if(valLen >= 2 && (val[0] == '\'' && val[valLen-1] == '\''))
		return 1;
	// Controllo se è un numero valido
	if(val[0] == '0') // Non puo iniziare per 0
		return 0;
	for(char *c = val; *c != 0; c++) {
		if(!isdigit(*c))
			return 0;
	}
	return 1;
}

bool parseCreate(char* query, query_t* parsed) {
	parsed->action = ACTION_CREATE;
	int readCount = 0;
	char cols[1000], table[1000], semicolon[2];
	memset(cols, 0, 1000);
	memset(table, 0, 1000);
	memset(semicolon, 0, 2);

	readCount = sscanf(query, " CREATE TABLE %s (%[^)]) %1[;]", table, cols, semicolon);
	if(readCount != 3) return 0;
	parsed->table = (char*)malloc(strlen(table) + 1);
	strcpy(parsed->table, table);

	char **colNames;
	size_t colCount = splitAndTrim(cols, ',', &colNames);

	for(int i = 0; i < colCount; i++) {
		if(!isValidName(colNames[i])) {
			freeStrings(&colNames, colCount);
			return 0;
		}
	}

	parsed->data = (query_data_t*)malloc((colCount+1) * sizeof(query_data_t));
	for(size_t i = 0; i < colCount; i++) {
		parsed->data[i] = newQueryData();
		parsed->data[i].colName = colNames[i];
	}
	parsed->data[colCount] = newQueryData(); // Inizializzo anche l'ultimo che è come terminatore

	free(colNames); // Libero solo la lista, le stringhe sono ancora utilizzate
	return 1;
}

bool parseInsert(char* query, query_t* parsed) {
	parsed->action = ACTION_INSERT;
	int readCount = 0;
	char cols[1000], table[1000], values[1000], semicolon[2];
	memset(cols, 0, 1000);
	memset(table, 0, 1000);
	memset(values, 0, 1000);
	memset(semicolon, 0, 2);

	readCount = sscanf(query, " INSERT INTO %s (%[^)]) VALUES (%[^)]) %1[;]", table, cols, values, semicolon);
	if(readCount != 4) return 0;
	parsed->table = (char*)malloc(strlen(table) + 1);
	strcpy(parsed->table, table);

	char **colNames;
	size_t colCount = splitAndTrim(cols, ',', &colNames);
	for(size_t i = 0; i < colCount; i++) {
		if(!isValidName(colNames[i])) {
			freeStrings(&colNames, colCount);
			return 0;
		}
	}

	char **valList;
	size_t valCount = splitAndTrim(values, ',', &valList);
	for(size_t i = 0; i < valCount; i++) {
		if(!isValidValue(valList[i])) {
			freeStrings(&colNames, colCount);
			freeStrings(&valList, valCount);
			return 0;
		}
	}

	if(valCount != colCount) {
		freeStrings(&colNames, colCount);
		freeStrings(&valList, valCount);
		return 0;
	}

	parsed->data = (query_data_t*)malloc((colCount+1) * sizeof(query_data_t));
	for(size_t i = 0; i < colCount; i++) {
		parsed->data[i] = newQueryData();
		parsed->data[i].colName = colNames[i];
		parsed->data[i].value = valList[i];
	}
	parsed->data[colCount] = newQueryData(); // Inizializzo anche l'ultimo che è come terminatore

	free(colNames); // Libero solo la lista, le stringhe sono ancora utilizzate
	free(valList);
	return 1;
}

bool parseSelect(char *query, query_t* parsed) {
	parsed->action = ACTION_SELECT;
	char cols[1000], table[1000], semicolon[2];
	memset(cols, 0, 1000);
	memset(table, 0, 1000);
	memset(semicolon, 0, 2);

	// Trovo la prima occorrenza di 'SELECT' e controllo che sia preceduta solo da spazi
	char *SELECT = strstr(query, "SELECT");
	if(SELECT == NULL)
		return 0;
	for(char *c = query; c < SELECT; c++) {
		if(!isblank(*c))
			return 0;
	}

	// Trovo la prima occorrenza di FROM: quello che è tra SELECT e FROM è la lista delle colonne
	char *FROM = strstr(query, "FROM");
	if(FROM == NULL)
		return 0;
	strncpy(cols, SELECT+6, FROM-(SELECT+6));

	// Splitto la lista di colonne e controllo che siano nomi validi
	char **colNames;
	size_t colCount = splitAndTrim(cols, ',', &colNames);
	// Se è presente solo la colonna * non controllo i nomi
	if(!(colCount == 1 && strcmp(colNames[0], "*") == 0)) {
		for(size_t i = 0; i < colCount; i++) {
			if(!isValidName(colNames[i])) {
				freeStrings(&colNames, colCount);
				return 0;
			}
		}
	}

	parsed->data = (query_data_t*)malloc((colCount+1) * sizeof(query_data_t));
	for(size_t i = 0; i < colCount; i++) {
		parsed->data[i] = newQueryData();
		parsed->data[i].colName = colNames[i];
	}
	parsed->data[colCount] = newQueryData();
	free(colNames); // Libero solo il puntatore, le stringhe sono in parsed->data

	// Prendo il nome della tabella
	char *tablePointer = readTrimWord(FROM+4, &(parsed->table), ';');
	if(!isValidName(parsed->table))
		return 0;

	char *filtPos;
	if((filtPos = strstr(tablePointer, "WHERE")) != NULL)
		parsed->filter = FILTER_WHERE;
	else if((filtPos = strstr(tablePointer, "ORDER")) != NULL)
		parsed->filter = FILTER_ORDERBY;
	else if((filtPos = strstr(tablePointer, "GROUP")) != NULL)
		parsed->filter = FILTER_GROUPBY;

	char *nextToRead = tablePointer;

	if(parsed->filter != FILTER_NONE) {
		// Controllo che sia preceduto solo da spazi
		for(char *c = tablePointer; c < filtPos; c++) {
			if(!isblank(*c)) {
				return 0;
			}
		}

		nextToRead = filtPos + 5; // Tutti e 3 i filtri sono lunghi 5 LOL
		if(!isblank(*nextToRead)) // Deve esserci uno spazio
			return 0;
	}

	if(parsed->filter == FILTER_ORDERBY || parsed->filter == FILTER_GROUPBY) {
		char *by = strstr(nextToRead, "BY");
		if(by == NULL) {
			return 0;
		}
		// Controllo che sia preceduto solo da spazi e che sia seguito da uno spazio
		for(char *c = nextToRead; c < by; c++) {
			if(!isblank(*c)) {
				return 0;
			}
		}
		if(!isblank(*(by + 2))) {
			return 0;
		}
		nextToRead = by + 2;
	}

	char *end = strstr(nextToRead, ";");
	if(end == NULL) {
		return 0;
	}

	// Tutti i filtri hanno per prima cosa il campo
	if(parsed->filter != FILTER_NONE) {
		// Prendo il nome della colonna eliminando prima tutti gli spazi, e fermandomi al primo spazio
		nextToRead = readTrimWord(nextToRead, &(parsed->filterField), ';');
		if(parsed->filterField == NULL || !isValidName(parsed->filterField)) {
			return 0;
		}
	}

	if(parsed->filter == FILTER_WHERE) {
		// Scarto i blank
		for(; *nextToRead != 0 && isblank(*nextToRead); nextToRead++);
		if(*nextToRead == 0) return 0;
		// Prendo l'operatore
		if(nextToRead[0] == '=' && nextToRead[1] == '=') {
			nextToRead += 2;
			parsed->op = OP_EQ;
		} else if(nextToRead[0] == '>') {
			nextToRead++;
			if(nextToRead[0] == '=') {
				parsed->op = OP_GE;
				nextToRead++;
			} else {
				parsed->op = OP_GT;
			}
		} else if(nextToRead[0] == '<') {
			nextToRead++;
			if(nextToRead[0] == '=') {
				parsed->op = OP_LE;
				nextToRead++;
			} else {
				parsed->op = OP_LT;
			}
		} else {
			return 0;
		}

		// Prendo il valore da confrontare eliminando prima tutti gli spazi, e fermandomi al primo spazio
		nextToRead = readTrimWord(nextToRead, &(parsed->filterValue), ';');
		if(parsed->filterValue == NULL || !isValidValue(parsed->filterValue)) {
			return 0;
		}
	} else if(parsed->filter == FILTER_ORDERBY) {
		// Prendo l'indicatore di ordinamento (ASC o DESC)
		char *tmp;
		nextToRead = readTrimWord(nextToRead, &tmp, ';');
		if(strcmp(tmp, "ASC") == 0) {
			parsed->op = OP_ASC;
			free(tmp);
		} else if(strcmp(tmp, "DESC") == 0) {
			parsed->op = OP_DESC;
			free(tmp);
		} else {
			free(tmp);
			return 0;
		}
	} else if(parsed->filter == FILTER_GROUPBY); // Do nothing (here just to readability)

	for(char *c = nextToRead; c < end; c++) {
		if(!isblank(*c)) {
			return 0;
		}
	}

	return 1;
}

bool parseQuery(char* query, query_t* parsed) {
	int readCount;

	char command[1000];
	readCount = sscanf(query, " %s ", command);
	if(readCount != 1) return 0;

	if(strcmp("CREATE", command) == 0) {
		return parseCreate(query, parsed);
	} else if(strcmp("INSERT", command) == 0) {
		return parseInsert(query, parsed);
	} else if(strcmp("SELECT", command) == 0) {
		return parseSelect(query, parsed);
	} else {
		parsed->data = (query_data_t*)malloc(sizeof(query_data_t));
		parsed->data[0] = newQueryData();
	}

	return 0;
}

/*************************************************************************************************
**                                                                                              **
**                    BLOCCO ORDINAMENTO DELLA TABELLA PER UNA DETERMINATA COLONNA              **
**                                                                                              **	
*************************************************************************************************/


// Verifica se il valore è un intero
bool identifyINT(char* elem) {
	for(char *c = elem; *c != 0; c++)
		if(!isdigit(*c))
			return false;
	return true;
}

// Trova la colonna da ordinare
int srcCOLUMNS(char** columns, char* src, int n_columns) {
	//trovo la parola
	for (int i = 0; i < n_columns; i++)
	{
		if (strcmp(columns[i], src) == 0)
			return i;
	}
	return -1;
}

// Converte i valori di una colonna in interi. Restituisce NULL se non ci riesce
int* columnToInt(const table_DB table, size_t id_column) {
	int *ints = (int*)malloc(table.n_row * sizeof(int));
	for(int i = 0; i < table.n_row; i++) {
		// Se non è un intero restituisco NULL
		char *val = table.data[i][id_column];
		if(!identifyINT(val)) {
			free(ints);
			return NULL;
		}
		ints[i] = atoi(val);
	}
	return ints;
}

//funzione ausiliare per l'ordinamento di  interi
void sortDBnumSWAP(int* a, int* b, char*** c, char*** d)
{
	int aux_int = *a;
	char **aux_char = *c;

	//swap
	*a = *b;
	*b = aux_int;
	*c = *d;
	*d = aux_char;
}

//funzione ausiliare per l'ordinamento di interi
int sortDBnumPARTITION(table_DB*DB, int vet[], int low, int high)//partition della funzione sortDBnumQUICKSORT
{
	int pivot = vet[high];    // pivot
	int i = (low - 1); //inizializzo a -1 perchè lo swap inizia con i++

	for (int j = low; j <= high - 1; j++)
	{
		//guardo se l'elemento che sto guardando è più piccolo del pivot
		if (vet[j] <= pivot)
		{
			i++;    //mi posiziono al primo numero non minore del pivot che ho trovato nel vettore (se non ho ancora trovato nessun elemento maggiore fa il cambio con sestesso)
			sortDBnumSWAP(&vet[i], &vet[j],&DB->data[i],&DB->data[j]);
		}
	}
	i++;
	sortDBnumSWAP(&vet[i], &vet[high] , &DB->data[i], &DB->data[high]);
	return i;
}

//funzione ausiliare per l'ordinamento di interi
void sortDBnumQUICKSORT(table_DB*DB, int vet[], int low, int high)//ordina per una colonna di interi una tabella
{
	if (low < high){//passo base
		//la partition restituisce il punto in cui spaccare nuovamente il vettore
		int p = sortDBnumPARTITION(DB, vet, low, high);

		//richiamo il quicksort su i due sotto vettori
		sortDBnumQUICKSORT(DB,vet, low, p - 1);//il pivot è già al suo posto quindi non lo considero più
		sortDBnumQUICKSORT(DB,vet, p + 1, high);
	}
}

//ordinamento di interi
bool sortDBnum(table_DB* DB, int id_column) {
	//trasformare la colonna di char in un vettore di interi
	int *vet = columnToInt(*DB, id_column);

	//ordinamento del vettore di interi(facciamo le stesse operazioni sulle righe del database)
	sortDBnumQUICKSORT(DB,vet,0,DB->n_row-1);

	free(vet);
	return true;
}

//funzione ausiliare per l'ordinamento di stringhe
void sortDBstrSWAP(char*** a, char*** b) {
	char **aux = *a;
	*a = *b;
	*b = aux;
}

//funzione ausiliare per l'ordinamento stringhe
int sortDBstrPARTITION(table_DB*DB, int id_columns, int low, int high)//partition della funzione sortDBnumQUICKSORT
{
	char *pivot = DB->data[high][id_columns];// pivot: vado a l'ultima riga e prendo la colonna che devo ordinare
	int i = (low - 1); //inizializzo a -1 perchè lo swap inizia con i++

	for (int j = low; j <= high - 1; j++)
	{
		//guardo se l'elemento che sto guardando è più piccolo del pivot
		if (strcmp(DB->data[j][id_columns],pivot)<=0)//ordinamento per ASCII
		{
			i++;    //mi posiziono al primo numero non minore del pivot che ho trovato nel vettore (se non ho ancora trovato nessun elemento maggiore fa il cambio con sestesso)
			sortDBstrSWAP(&DB->data[i], &DB->data[j]);
		}
	}
	i++;
	sortDBstrSWAP(&DB->data[i], &DB->data[high]);
	return i;
}

//ordiamento di stringhe
void sortDBstrQUICKSORT(table_DB* DB, int id_columns, int low, int high){
	if (low < high)//passo base
	{
		//la partition restituisce il punto in cui spaccare nuovamente il vettore
		int p = sortDBstrPARTITION(DB, id_columns, low, high);

		//richiamo il quicksort su i due sotto vettori
		sortDBstrQUICKSORT(DB, id_columns, low, p - 1);//il pivot è già al suo posto quindi non lo considero più
		sortDBstrQUICKSORT(DB, id_columns, p + 1, high);
	}
}

// Ordina le righe di una tabella in base alla colonna specificata
bool sortDB(table_DB* DB, char *column) {
	// Indice delle colonna
	int id_column = srcCOLUMNS(DB->columns, column, DB->n_columns);
	if (id_column == -1)
		return false;

	// Scopro il tipo della colonna
	bool typeINT = identifyINT(DB->data[1][id_column]);

	// Ordino per numero o per stringa
	if (typeINT == true) {
		return sortDBnum(DB, id_column);
	} else {
		sortDBstrQUICKSORT(DB, id_column, 0, DB->n_row);
		return true;
	}
}

/*************************************************************************************************
**                                                                                              **
**              FINE BLOCCO ORDINAMENTO DELLA TABELLA PER UNA DETERMINATA COLONNA               **
**                                                                                              **
*************************************************************************************************/



/*************************************************************************************************
**                                                                                              **
**                                        BLOCCO SELECTION                                      **
**                                                                                              **
*************************************************************************************************/

// Compara due stringhe usando l'operatore di un filtro
bool compareValuesStr(char* a, char* b, filter_op_t op) {
	switch (op) {
		case OP_EQ://uguale
			return strcmp(a, b) == 0;
		case OP_GT://maggiore
			return strcmp(a, b) >  0;
		case OP_GE://maggiore uguale
			return strcmp(a, b) >= 0;
		case OP_LT://minore
			return strcmp(a, b) <  0;
		case OP_LE://minore uguale
			return strcmp(a, b) <= 0;
	}
	return false; // Non dovrebbe mai arrivarci
}

// Compara due interi usando l'operatore di un filtro
bool compareValuesInt(int a, int b, filter_op_t op) {
	switch (op) {
		case OP_EQ://uguale
			return a == b;
		case OP_GT://maggiore
			return a >  b;
		case OP_GE://maggiore uguale
			return a >= b;
		case OP_LT://minore
			return a <  b;
		case OP_LE://minore uguale
			return a <= b;
	}
	return false; // Non dovrebbe mai arrivarci
}

//WHERE
bool selectWHERE(query_t query, table_DB* table) {
	int id_column = srcCOLUMNS(table->columns, query.filterField, table->n_columns);
	if (id_column == -1)
		return false;

	bool isIntQuery = identifyINT(query.filterValue);
	bool isIntTable = identifyINT(table->data[1][id_column]);
	if (isIntQuery != isIntTable)
		return false;
	
	int* vet = NULL;
	int aux;
	// Se sono interi converto tutto in int
	if (isIntTable == true) {
		// Trasformo la colonna di char in un vettore di interi 
		vet = columnToInt(*table, id_column);
		aux = atoi(query.filterValue);
	}

	int id_next = 0; // Indice della prossima riga da inserire
	// Scorro tutta la tabella e costruisco la nuova tabella eliminando gli elementi che non rispettano la condizione
	for (int i = 0; i < table->n_row; i++) {
		bool confronto = false;
		if (isIntTable == true) {
			confronto = compareValuesInt(vet[i], aux, query.op);
		} else {
			confronto = compareValuesStr(table->data[i][id_column], query.filterValue, query.op);
		}
		
		// Sovrascrivo se rispetta il confronto
		if (confronto) {
			if(id_next < i) { // Se dovrei sostituire la riga con se stessa non faccio niente
				freeStrings(&table->data[id_next], table->n_columns);
				table->data[id_next] = table->data[i];
				table->data[i] = NULL;
			}
			id_next++;
		}
	}

	// Aggiorno il numero righe
	for(int i = id_next; i < table->n_row; i++) {
		freeStrings(&table->data[i], table->n_columns);
	}
	table->n_row = id_next;
	table->data = (char***)realloc(table->data, table->n_row * sizeof(char**));
	if(vet)
		free(vet);
	return true;
}

//ORDER BY desc vale true quando è desc; desc vale false quando è asc
bool selectORDERby(char*order_by, int desc, table_DB*DB) {
	if (sortDB(DB, order_by) == false)
		return false;
	
	if (desc == OP_DESC)//inverti tabella
	{
		for(int i = 0, last = DB->n_row - 1; last > i; i++, last--)
		{
			char **aux = DB->data[i];
			DB->data[i] = DB->data[last];
			DB->data[last] = aux;
		}
	}
	return true;
}

//GROUP BY
int* selectGROUPby(char* group_by, table_DB* DB){//modifica la tabella ragruppando per colonna, restituisce un vettore di interi lungo n_row del DB oppure NULL in caso di errore
	int id_next = 0;//che si posiziona dove si può sovrascrivere il prossimo elemento
	int id_columns = srcCOLUMNS(DB->columns, group_by, DB->n_columns);
	int *vet;

	if (id_columns == -1)
		return NULL;
	vet = (int*)malloc(DB->n_row * sizeof(int));

	//ordina
	if (sortDB(DB, group_by) == false)
		return false;

	//group
	int count_group = 1;//contatore delle righe che hanno la colonna uguale
	char *last = NULL;
	for (int i = 0; i < DB->n_row; i++) {
		if(i == 0 || strcmp(DB->data[i][id_columns], last) == 0) {
			count_group++;//conto le righe che hanno le colonne uguali
		} else {//se sono diversi raggruppo
			vet[id_next] = count_group;//carico il numero di righe uguali nel vettore
			DB->data[id_next] = DB->data[i];//carico la riga nella prima disponibile 
			id_next++;//incremento l'indice in cui andrò a scrivere/sovrascrivere al prossimo raggruppo
			count_group = 1;//inizializzo di nuovo il conteggio
			last = DB->data[i][id_columns];//carico la nuova stringa da confrontare 
		}
	}
	vet[id_next] = count_group;//carico il numero di righe uguali nel vettore

	DB->n_row = ++id_next;//aggiorno il nuovo numero righe
	return vet;
}

bool executeSelect(query_t query, table_DB* table, table_DB* result) {
	// WHERE ha bisogno anche dei dati non selezionati
	if(query.filter == FILTER_WHERE) {
		if(!selectWHERE(query, table))
			return false;
	}

	// Copio il nome della tabella
	*result = newTable();
	result->table_name = (char*)malloc(strlen(table->table_name) + 1);
	strcpy(result->table_name, table->table_name);

	bool allColumns = query.data[0].colName[0] == '*'; // Solo per scrivere meno
	if(allColumns) {
		result->n_columns = table->n_columns;
	} else {
		// Conto le colonne della query
		for(query_data_t* d = query.data; d->colName != NULL; d++, result->n_columns++);
	}
	result->columns = (char**)malloc(result->n_columns * sizeof(char*));

	// Copio le colonne e trovo gli id corrispondenti
	int colIds[result->n_columns];
	for(int i = 0; i < result->n_columns; i++) {
		char* colName = allColumns ? table->columns[i] : query.data[i].colName; // Se '*', uso le colonne della tabella stessa
		result->columns[i] = (char*)malloc(strlen(colName) + 1);
		strcpy(result->columns[i], colName);

		colIds[i] = allColumns ? i : srcCOLUMNS(table->columns, query.data[i].colName, table->n_columns);
		if(colIds[i] == -1) {
			return false;
		}
	}

	// Copio i valori della tabella
	result->n_row = table->n_row;
	result->data = (char***)malloc(result->n_row * sizeof(char**));
	for(int i = 0; i < result->n_row; i++) {
		result->data[i] = (char**)malloc(result->n_columns * sizeof(char*));
		for(int j = 0; j < result->n_columns; j++) {
			char* value = table->data[i][colIds[j]];
			result->data[i][j] = (char*)malloc(strlen(value) + 1);
			strcpy(result->data[i][j], value);
		}
	}

	if(query.filter == FILTER_GROUPBY) {
		// applyGroupBy(query, table, &result)
	} else if(query.filter == FILTER_ORDERBY) {
		// applyOrderBy(query, table, &result);
	}

	return true;
}

bool executeCreate(query_t query, table_DB* result) {
	// Copio il nome della tabella
	result->table_name = (char*)malloc(strlen(query.table) + 1);
	strcpy(result->table_name, query.table);

	// Copio le colonne
	for(query_data_t* d = query.data; d->colName != NULL; d++, result->n_columns++);
	if(result->n_columns == 0)
		return false;
	result->columns = (char**)malloc(result->n_columns * sizeof(char*));
	for(int i = 0; i < result->n_columns; i++) {
		result->columns[i] = (char*)malloc(strlen(query.data[i].colName) + 1);
		strcpy(result->columns[i], query.data[i].colName);
	}
	return true;
}

bool executeInsert(query_t query, table_DB* table) {
	// Check di validita: stessi nomi colonne nello stesso ordine, tanti valori quanti colonne
	int n_queryColumns = 0;
	// Conto quante colonne ci sono e che tutte abbiano sia nome che valore
	for(query_data_t* d = query.data; d->colName != NULL || d->value != NULL; d++, n_queryColumns++) {
		if(d->colName == NULL || d->value == NULL)
			return false;
	}
	if(n_queryColumns != table->n_columns)
		return false;

	table->n_row++;
	table->data = (char***)realloc(table->data, table->n_row * sizeof(char**));
	table->data[table->n_row - 1] = (char**)malloc(table->n_columns * sizeof(char*));
	for(int i = 0; i < table->n_columns; i++) {
		table->data[table->n_row - 1][i] = (char*)malloc(strlen(query.data[i].value) + 1);
		strcpy(table->data[table->n_row - 1][i], query.data[i].value);
	}
	return true;
}

/*************************************************************************************************
**                                                                                              **
**                                      FINE BLOCCO SELECTION                                   **
**                                                                                              **
*************************************************************************************************/

/*************************************************************************************************
**                                                                                              **
**                                   BLOCCO GESTIONE FILESYSTEM                                 **
**                                                                                              **
*************************************************************************************************/

// Converte una query in stringa
char* queryString(query_t query) {
	char* out = (char*)malloc(1024);
	if(query.action == ACTION_SELECT) {
		strcpy(out, "SELECT ");
		for(query_data_t* d = query.data; d->colName != NULL; d++) {
			if(d != query.data)
				strcat(out, ", ");
			strcat(out, d->colName);
		}
		strcat(out, " FROM ");
		strcat(out, query.table);

		if(query.filter == FILTER_WHERE) {
			strcat(out, " WHERE ");
			strcat(out, query.filterField);
			if(query.op == OP_EQ)
				strcat(out, " == ");
			else if(query.op == OP_GT)
				strcat(out, " > ");
			else if(query.op == OP_GE)
				strcat(out, " >= ");
			else if(query.op == OP_LT)
				strcat(out, " < ");
			else if(query.op == OP_LE)
				strcat(out, " <= ");
			strcat(out, query.filterValue);
		} else if(query.filter == FILTER_GROUPBY) {
			strcat(out, " GROUP BY ");
			strcat(out, query.filterField);
		} else if(query.filter == FILTER_ORDERBY) {
			strcat(out, " ORDER BY ");
			strcat(out, query.filterField);
			if(query.op == OP_ASC)
				strcat(out, " ASC");
			else if(query.op == OP_DESC)
				strcat(out, " DESC");
		}
		strcat(out, ";");
	} else if(query.action == ACTION_CREATE) {
		strcpy(out, "CREATE TABLE ");
		strcat(out, query.table);
		strcat(out, " (");
		for(query_data_t* d = query.data; d->colName != NULL; d++) {
			if(d != query.data)
				strcat(out, ", ");
			strcat(out, d->colName);
		}
		strcat(out, ");");
	} else if(query.action == ACTION_INSERT) {
		strcpy(out, "INSERT INTO ");
		strcat(out, query.table);
		strcat(out, " (");
		for(query_data_t* d = query.data; d->colName != NULL; d++) {
			if(d != query.data)
				strcat(out, ", ");
			strcat(out, d->colName);
		}
		strcat(out, ") VALUES (");
		for(query_data_t* d = query.data; d->value != NULL; d++) {
			if(d != query.data)
				strcat(out, ", ");
			strcat(out, d->value);
		}
		strcat(out, ");");
	}
	return out;
}

// Genera il filename per leggere/salvare una tabella
char* tableFilename(char* tableName) {
	char* fileName = (char*)malloc(strlen(tableName) + 5); // 5 = .txt\0
	strcpy(fileName, tableName);
	strcat(fileName, ".txt");
	return fileName;
}

// Converte la definizione di una tabella (lista campi) in stringa
char* tableHeaderString(table_DB table) {
	// "TABLE name COLUMNS "
	size_t totalSize = 6 + strlen(table.table_name) + 9;
	size_t columnsStart = totalSize; // Indice della stringa dove inizia la lista di colonne
	for(size_t i = 0; i < table.n_columns; i++) {
		totalSize += strlen(table.columns[i]) + 1;
	}
	totalSize++; // Carattere terminatore
	char *header = (char*)malloc(totalSize);

	sprintf(header, "TABLE %s COLUMNS ", table.table_name);
	char *nextToWrite = header + columnsStart;
	for(size_t i = 0; i < table.n_columns; i++) {
		strcpy(nextToWrite, table.columns[i]);
		nextToWrite += strlen(table.columns[i]);
		if(i < table.n_columns - 1) {
			*nextToWrite = ',';
		} else {
			*nextToWrite = ';';
		}
		nextToWrite++;
	}
	*nextToWrite = 0;

	return header;
}

// Converte in stringa una riga di una tabella
char* tableRowString(table_DB table, size_t id_row) {
	// "ROW "
	size_t totalSize = 4;
	size_t valuesStart = totalSize; // Indice della stringa dove inizia la lista di valori
	for(size_t i = 0; i < table.n_columns; i++) {
		totalSize += strlen(table.data[id_row][i]) + 1;
	}
	totalSize++; // Carattere terminatore
	char *row = (char*)malloc(totalSize);

	sprintf(row, "ROW ");
	char *nextToWrite = row + valuesStart;
	for(size_t i = 0; i < table.n_columns; i++) {
		strcpy(nextToWrite, table.data[id_row][i]);
		nextToWrite += strlen(table.data[id_row][i]);
		if(i < table.n_columns - 1) {
			*nextToWrite = ',';
		} else {
			*nextToWrite = ';';
		}
		nextToWrite++;
	}
	*nextToWrite = 0;
	return row;
}

// Converte in stringa un'intera tabella
char* tableString(table_DB table) {
	char* header = tableHeaderString(table);
	char** rows = (char**)malloc(table.n_row * sizeof(char*));
	size_t totalSize = strlen(header) + 1;
	for(size_t i = 0; i < table.n_row; i++) {
		rows[i] = tableRowString(table, i);
		totalSize += strlen(rows[i]) + 1;
	}
	totalSize++; // Carattere terminatore

	char* tableStr = (char*)malloc(totalSize);
	strcpy(tableStr, header);
	strcat(tableStr, "\n");
	for(size_t i = 0; i < table.n_row; i++) {
		strcat(tableStr, rows[i]);
		strcat(tableStr, "\n");
	}
	tableStr[totalSize-1] = 0;

	free(header);
	freeStrings(&rows, table.n_row);
	return tableStr;
}

// Salva su file una tabella
void saveTable(table_DB table) {
	char* fileName = tableFilename(table.table_name);
	FILE *out = fopen(fileName, "w");

	char* tableStr = tableString(table);
	fputs(tableStr, out);

	free(fileName);
	free(tableStr);
	fclose(out);
}

// Carica da file una tabella
bool loadTable(char* name, table_DB* table) {
	char* fileName = tableFilename(name);
	FILE* in = fopen(fileName, "r");
	if(in == NULL) {
		free(fileName);
		return false;
	}

	char* nextToRead = NULL;
	char* tmpPtr;
	const size_t bufSize = 1024;
	char tmpBuf[bufSize];
	fgets(tmpBuf, bufSize, in);
	long rowsStart = ftell(in);

	nextToRead = strchr(tmpBuf, ' ') + 1;
	tmpPtr = strchr(nextToRead, ' ');
	table->table_name = (char*)malloc((tmpPtr - nextToRead) + 1);
	strncpy(table->table_name, nextToRead, (tmpPtr - nextToRead));
	table->table_name[(tmpPtr - nextToRead)] = 0;

	nextToRead = strchr(tmpPtr + 1, ' ') + 1;
	*(strchr(nextToRead, ';')) = 0;
	table->n_columns = splitAndTrim(nextToRead, ',', &table->columns);

	//Contiamo le righe
	for(char c; (c = fgetc(in)) != EOF; table->n_row += (c == '\n'));
	table->data = (char***)malloc(table->n_row * sizeof(char**));

	fseek(in, rowsStart, SEEK_SET);
	for(int i = 0; i < table->n_row; i++) {
		fgets(tmpBuf, bufSize, in);
		nextToRead = strchr(tmpBuf, ' ') + 1;
		*(strchr(nextToRead, ';')) = 0;
		splitAndTrim(nextToRead, ',', &(table->data[i]));
	}

	free(fileName);
	fclose(in);
	return true;
}

// Salva nel file delle select la query e il risultato indicati
bool saveSelect(query_t query, table_DB result) {
	char* queryStr = queryString(query);
	char* tableStr = tableString(result);

	FILE* out = fopen("query_results.txt", "a");
	fprintf(out, "%s\n%s\n", queryStr, tableStr);

	free(queryStr);
	free(tableStr);
	fclose(out);
	return true;
}

/*************************************************************************************************
**                                                                                              **
**                                 FINE BLOCCO GESTIONE FILESYSTEM                              **
**                                                                                              **
*************************************************************************************************/

/*************************************************************************************************
**                                                                                              **
**                                        MAIN: EXECUTEQUERY                                    **
**                                                                                              **
*************************************************************************************************/

bool executeQuery(char*str) {
	int ok = true;

	query_t query = newQuery();
	if(!parseQuery(str, &query)) {
		ok = false;
	}

	if(ok && query.action == ACTION_SELECT) {
		table_DB sourceTable = newTable();
		if(!loadTable(query.table, &sourceTable)) {
			ok = false;
		}
		table_DB result = newTable();
		if(ok && executeSelect(query, &sourceTable, &result)) {
			saveSelect(query, result);
		} else {
			ok = false;
		}

		freeTable(&sourceTable);
		freeTable(&result);
	} else if(ok && query.action == ACTION_CREATE) {
		table_DB result = newTable();
		if(executeCreate(query, &result)) {
			saveTable(result);
		} else {
			ok = false;
		}
		freeTable(&result);
	} else if(ok && query.action == ACTION_INSERT) {
		table_DB table = newTable();
		if(loadTable(query.table, &table)) {
			ok = executeInsert(query, &table);
			saveTable(table);
		} else {
			ok = false;
		}

		freeTable(&table);
	}

	freeQuery(&query);

	return ok;
}
