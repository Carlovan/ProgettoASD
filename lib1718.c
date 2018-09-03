#include "lib1718.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



// Chiama free su una lista di stringhe
void freeStrings(char*** l, size_t n) {
	for(int i = 0; i < n; i++)
		free((*l)[i]);
	free(*l);
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
			if(nextToRead[1] == '=') {
				parsed->op = OP_GE;
				nextToRead++;
			} else {
				parsed->op = OP_GT;
			}
		} else if(nextToRead[0] == '<') {
			nextToRead++;
			if(nextToRead[1] == '=') {
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

//ordinamento di interi
bool sortDBnum(table_DB* DB, int id_columns) {
	//trasformare la colonna di char in un vettore di interi 
	int *vet = (int*)malloc(DB->n_row * sizeof(int));

	//carichiamo i valori nel vettore
	for(int i = 0; i < DB->n_row; i++) {
		vet[i] = atoi(DB->data[i][id_columns]);
	}

	//ordinamento del vettore di interi(facciamo le stesse operazioni sulle righe del database)
	sortDBnumQUICKSORT(DB,vet,0,DB->n_row-1);//-1 perché il vettore parte da 0 

	free(vet);
	return true;
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

//funzione ausiliare per l'ordinamento di stringhe
void sortDBstrSWAP(char*** a, char*** b) {
	char **aux = *a;
	*a = *b;
	*b = aux;
}

//trova la colonna da ordinare
int srcCOLUMNS(char** columns, char* src, int n_columns) {
	//trovo la parola 
	for (int i = 0; i < n_columns; i++)
	{
		if (strcmp(columns[i], src) == 0)
			return i;
	}
	return -1;
}

//verfifica se la colonna è un intero
bool identifyINT(char* elem) {
	for(char *c = elem; *c != 0; c++)
		if(!isdigit(*c))
			return false;
	return true;
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


//SENZA FILTRI(stampi direttamente la tabella senza fare niente solo con le colonne che sono richieste 

//WHERE
bool selectWHERE(char *whereCOLUMN, char *valore, int operator, table_DB*DB) {
	int id_column = srcCOLUMNS(DB->columns, whereCOLUMN, DB->n_columns);
	if (id_column == -1)
		return false;

	bool typeINTa = identifyINT(valore);
	bool typeINTb = identifyINT(DB->data[1][id_column]);
	if (typeINTa != typeINTb)
		return false;
	
	int *vet;
	int aux;
	//se sono interi bisogna creare il vettore di interi per velocizzare i confronti
	if (typeINTb == true) {
		//trasformare la colonna di char in un vettore di interi 
		vet = (int*)malloc(DB->n_row * sizeof(int));

		//carichiamo i valori nel vettore
		for (int i = 0; i < DB->n_row; i++) {
			vet[i] = atoi(DB->data[i][id_column]);
		}
		aux = atoi(valore);
	}

	int id_next = 0;//dove andrò a inserire la prossima riga
	bool confronto = false;
	//mi scorro tutto il DB e costruisco il nuovo DB eliminato gli elementi che non rispettano la condizione
	for (int i = 0; i < DB->n_row; i++) {
		if (typeINTa == true) {//confronto per numeri interi
			switch (operator) {
			case OP_EQ://uguale
				confronto = vet[i] == aux;
				break;
			case OP_GT://maggiore
				confronto = vet[i] > aux;
				break;
			case OP_GE://maggiore uguale
				confronto = vet[i] >= aux;
				break;
			case OP_LT://minore
				confronto = vet[i] < aux;
				break;
			case OP_LE://minore uguale
				confronto = vet[i] <= aux;
				break;
			}
		}
		else {//confronto tra stringhe
			switch (operator) {
			case OP_EQ://uguale
				confronto = strcmp(DB->data[i][id_column], valore) == 0;
				break;
			case OP_GT://maggiore
				confronto = strcmp(DB->data[i][id_column], valore) > 0;
				break;
			case OP_GE://maggiore uguale
				confronto = strcmp(DB->data[i][id_column], valore) >= 0;
				break;
			case OP_LT://minore
				confronto = strcmp(DB->data[i][id_column], valore) < 0;
				break;
			case OP_LE://minore uguale
				confronto = strcmp(DB->data[i][id_column], valore) <= 0;
				break;
			}

		}
		
		//sovrascrivo se rispetta il confronto
		if (confronto == true) {
			confronto = false;
			DB->data[id_next] = DB->data[confronto];
			id_next++;
		}
	}

	//aggiorno il numero righe 
	DB->n_row = id_next;
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
		}
	}
	vet[id_next] = count_group;//carico il numero di righe uguali nel vettore

	DB->n_row = id_next;//aggiorno il nuovo numero righe
	return vet;
}

/*************************************************************************************************
**                                                                                              **
**                                      FINE BLOCCO SELECTION                                   **
**                                                                                              **
*************************************************************************************************/

/*************************************************************************************************
**                                                                                              **
**                                        MAIN: EXECUTEQUERY                                    **
**                                                                                              **
*************************************************************************************************/

bool executeQuery(char*str) {
	//dichiarazione variabili
	int *group_by;
	bool error;
	query_t *query;
	table_DB *DB;

	//inizializazione delle variabili
	query = (query_t*)malloc(sizeof(query_t));	
	if (query == NULL) 
		return false;
	DB = (table_DB*)malloc(sizeof(table_DB));
	if (DB == NULL)
		return false;
	//error = parsed(str, query);//costruisce la struttura query_t 
	if (!error) 
		return false;
	//error = buildTable(query->table,DB);//costruisce la struttura tabella partendo dal file
	
	switch (query->action){
		case ACTION_CREATE:
			//funzione action create
			//funzione stampa create 
			break;
		case ACTION_INSERT:
			//funzione action insert
			//funzione stampa insert
			break;
		case ACTION_SELECT:
			switch (query->filter){
				case FILTER_NONE:
					//action/stampa selection none
					break;
				case FILTER_WHERE:
					error = selectWHERE(query->filterField, query->filterValue,query->op, DB);
					break;
				case FILTER_ORDERBY:
					error=selectORDERby(query->filterField, query->op, DB);
					if (!error)
						return false;
					break;
				case FILTER_GROUPBY:
					group_by = selectGROUPby(query->filterField, DB);
					if (group_by == NULL)
						return false;
					//stampa speciale<------ATTENZIONE------->
					//stampaGroupBy(int *group_by,table_DB *DB,int id_collumns);//tipo di prototipo di funzione per la stampa del groupBy, la funzione group by restituisce un vettore di interi l'indice i-esimo del vettore corrisponde a quante volte si ripete la riga iesima del DB
					break;
				if (query->filter != FILTER_GROUPBY)
						//stampa selection; dato che io nelle selectiono vado  a modificare il DB basta stampare il DB per stampare il risultato della selection
					;
			}
			break;
	}
	return true;
}