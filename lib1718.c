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
		free(q->table);//forse da rivedere %
	if(q->data != NULL) {
		for(query_data_t *i = q->data; i->colName != NULL; i++) {
			free(i->colName);
			if(i->value != NULL)//se il colName esiste allora esiste anche il Value%
				free(i->value);
		}
		free(q->data);
	}
}

// Restituisce un nuovo oggetto query_t
query_t newQuery() {
	query_t q = {0, NULL, NULL};
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
	// Conto quanti pezzi ci sono, considerando delimitatori in fila come uno unico
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
	for(size_t i = 0; i < colCount; i++) {
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
	parsed->data[colCount] = newQueryData();
	free(colNames); // Libero solo il puntatore, le stringhe sono in parsed->data

	// Prendo il nome della tabella
	char *tablePointer;
	for(tablePointer = FROM+4; *tablePointer != 0 && isblank(*tablePointer); tablePointer++);
	for(size_t i = 0; *tablePointer != 0 && !isblank(*tablePointer) && *tablePointer != ';'; i++, tablePointer++) {
		table[i] = *tablePointer;
	}
	if(!isValidName(table))
		return 0;

	parsed->filter = 0;
	char *filtPos;
	if((filtPos = strstr(tablePointer, "WHERE")) != NULL)
		parsed->filter = FILTER_WHERE;
	else if((filtPos = strstr(tablePointer, "ORDER")) != NULL)
		parsed->filter = FILTER_ORDERBY;
	else if((filtPos = strstr(tablePointer, "GROUP")) != NULL)
		parsed->filter = FILTER_GROUPBY;

	char *nextToRead = tablePointer;

	if(parsed->filter != 0) {
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


	char *end = strstr(tablePointer, ";");
	if(end == NULL) {
		return 0;
	}
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

//dichiarazione di tutte le funzioni per eliminare i warning
bool sortDB(table_DB* DB, char *columns);
bool sortDBnum(table_DB* DB, int id_columns);
void sortDBnumQUICKSORT(table_DB*DB, int vet[], int low, int high);
int sortDBnumPARTITION(table_DB*DB, int vet[], int low, int high);
void sortDBnumSWAP(int* a, int* b, char***c, char***d);
void sortDBstrQUICKSORT(table_DB* DB, int id_columns, int low, int high);
int sortDBstrPARTITION(table_DB*DB, int id_columns, int low, int high);
void sortDBstrSWAP(char*** a, char*** b);
int srcCOLUMNS(char** columns, char* src);
bool identifyINT(char* elem);


//master
bool sortDB(table_DB* DB, char *columns) {
	int id_columns;
	bool typeINT, error;

	//cerca indice della tabella
	id_columns = srcCOLUMNS(DB->columns, columns,DB->n_columns);
	if (id_columns == -1)
		return false;

	//scopro il tipo della colonna
	typeINT = identifyINT(DB->data[1][id_columns]);

	//ordino per numero o per stringa
	if (typeINT == true)
		error = sortDBnum(DB, id_columns);
	else
		sortDBstrQUICKSORT(DB, id_columns,0,DB->n_row);
	if (error == false)
		return false;
	return true;
}

//ordinamento di interi
bool sortDBnum(table_DB* DB, int id_columns) {
	//dichiarazioni delle variabili
	int* vet,i;

	//inizializazione delle variabili
	i = 0;

	//trasformare la colonna di char in un vettore di interi 
	vet = (int*)malloc(DB->n_row * sizeof(int));
	if (vet == NULL)
		return false;

	//carichiamo i valori nel vettore
	while (i < DB->n_row) {
		vet[i] = atoi(DB->data[i][id_columns]);
		i++;
	}

	//ordinamento del vettore di interi(facciamo le stesse operazioni sulle righe del database)
	sortDBnumQUICKSORT(DB,vet,0,DB->n_row-1);//-1 perché il vettore parte da 0 

	return true;
}

//funzione ausiliare per l'ordinamento di interi
void sortDBnumQUICKSORT(table_DB*DB, int vet[], int low, int high)//ordina per una colonna di interi una tabella
{
	int p;
	if (low < high){//passo base
		//la partition restituisce il punto in cui spaccare nuovamente il vettore
		p = sortDBnumPARTITION(DB, vet, low, high);

		//richiamo il quicksort su i due sotto vettori
		sortDBnumQUICKSORT(DB,vet, low, p - 1);//il pivot è già al suo posto quindi non lo considero più
		sortDBnumQUICKSORT(DB,vet, p + 1, high);
	}
}

//funzione ausiliare per l'ordinamento di interi
int sortDBnumPARTITION(table_DB*DB, int vet[], int low, int high)//partition della funzione sortDBnumQUICKSORT
{
	//dichiarazione delle variabili
	int pivot, j, i;

	//inizializzo le variabili
	pivot = vet[high];    // pivot
	i = (low - 1); //inizializzo a -1 perchè lo swap inizia con i++


	for (j = low; j <= high - 1; j++)
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
	return (i);

}

//funzione ausiliare per l'ordinamento di  interi
void sortDBnumSWAP(int* a, int* b, char***c, char***d)
{
	//dichiarazione delle variabili
	int aux_int;
	char **aux_char;

	//inizializazione
	aux_int = *a;
	aux_char = *c;

	//swap
	*a = *b;
	*b = aux_int;
	*c = *d;
	*d = aux_char;
}

//ordiamento di stringhe
void sortDBstrQUICKSORT(table_DB* DB, int id_columns, int low, int high){
	int p;
	if (low < high)//passo base
	{
		//la partition restituisce il punto in cui spaccare nuovamente il vettore
		p = sortDBstrPARTITION(DB, id_columns, low, high);

		//richiamo il quicksort su i due sotto vettori
		sortDBstrQUICKSORT(DB, id_columns, low, p - 1);//il pivot è già al suo posto quindi non lo considero più
		sortDBstrQUICKSORT(DB, id_columns, p + 1, high);
	}
	
	
	
}

//funzione ausiliare per l'ordinamento stringhe
int sortDBstrPARTITION(table_DB*DB, int id_columns, int low, int high)//partition della funzione sortDBnumQUICKSORT
{
	//dichiarazione delle variabili
	int j, i;
	char*pivot;

	//inizializzo le variabili
	pivot = DB->data[high][id_columns];// pivot: vado a l'ultima riga e prendo la colonna che devo ordinare
	i = (low - 1); //inizializzo a -1 perchè lo swap inizia con i++


	for (j = low; j <= high - 1; j++)
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
	return (i);
}

//funzione ausiliare per l'ordinamento di stringhe
void sortDBstrSWAP(char*** a, char*** b) {
	char **aux;
	aux = *a;
	*a = *b;
	*b = aux;
}

//trova la colonna da ordinare
int srcCOLUMNS(char** columns, char* src, int n_columns) {
	
	//dichiarazione delle variabili
	int i;
	bool trovato;

	//inizializazione delle variabili
	i = 0;
	trovato = 0;

	//trovo la parola 
	while (i < n_columns && !trovato)
	{
		if (strcmp(columns[i], src) == 0)
			trovato = true;
		else
			i++;
	}

	//se la trovo restiuisco l'indice se non la trovo restituisco -1
	if (trovato)
		return i;
	return -1;
}

//verfifica se la colonna è un intero
bool identifyINT(char* elem) {

	//dichiarazione delle variabili
	bool ISaNUMBER = true;
	int i = 0;

	//guardo se è un intero o una stringa
	while (i < strlen(elem) && ISaNUMBER)
	{
	if (!isdigit(elem[i]))
		ISaNUMBER = false;
	i++;
	}

	//se è un numero intero allora arriva alla fine della stringa altrimenti si ferma prima
	if(i==strlen(elem))
		return true;
	return false;
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
bool selectWHERE(char *whereCOLUMNS, char *valore, int operatore, table_DB*DB) {
	//dichiarazione
	bool typeINTa, typeINTb,confronto;
	int id_columns,i,*vet,aux,id_where;

	//inizializazione
	confronto = false;
	i = 0;
	id_where = 0;//dove andrò a sovrascrivere
	id_columns = srcCOLUMNS(DB->columns, whereCOLUMNS, DB->n_columns);
	if (id_columns == -1)
		return false;
	typeINTa = identifyINT(valore);
	typeINTb = identifyINT(DB->data[1][id_columns]);
	if (typeINTa != typeINTb)
		return false;
	

	//se sono interi bisogna creare il vettore di interi per velocizzare i confronti
	if (typeINTb == true) {
		//trasformare la colonna di char in un vettore di interi 
		vet = (int*)malloc(DB->n_row * sizeof(int));
		if (vet == NULL)
			return false;

		//carichiamo i valori nel vettore
		while (i < DB->n_row) {
			vet[i] = atoi(DB->data[i][id_columns]);
			i++;
		}
	}
	aux = atoi(valore);

	//mi scorro tutto il DB e costruisco il nuovo DB eliminato gli elementi che non rispettano la condizione
	while (i < DB->n_row) {
		//inizio il confronto
		if (typeINTa == true) {//confronto per numeri interi
			switch (operatore) {
			case OP_EQ://uguale
				if (vet[i] == aux) {
					confronto = true;
				}
				break;
			case OP_GT://maggiore
				if (vet[i] > aux) {
					confronto = true;
				}
				break;
			case OP_GE://maggiore uguale
				if (vet[i]>= aux) {
					confronto = true;
				}
				break;
			case OP_LT://minore
				if (vet[i] < aux) {
					confronto = true;
				}
				break;
			case OP_LE://minore uguale
				if (vet[i] <= aux) {
					confronto = true;
				}
				break;
			}
		}
		else {//confronto tra stringhe
			switch (operatore) {
			case OP_EQ://uguale
				if (strcmp(DB->data[i][id_columns],valore) == 0) {
					confronto = true;
				}
				break;
			case OP_GT://maggiore
				if (strcmp(DB->data[i][id_columns], valore)> 0) {
					confronto = true;
				}
				break;
			case OP_GE://maggiore uguale
				if (strcmp(DB->data[i][id_columns], valore) >= 0) {
					confronto = true;
				}
				break;
			case OP_LT://minore
				if (strcmp(DB->data[i][id_columns],valore) < 0) {
					confronto = true;
				}
				break;
			case OP_LE://minore uguale
				if (strcmp(DB->data[i][id_columns],valore) <= 0) {
					confronto = true;
				}
				break;
			}

		}
		
		//sovrascrivo se rispetta il confronto
		if (confronto == true) {
			confronto = false;
			DB->data[id_where] = DB->data[confronto];
			id_where++;
		}
	}

	//aggiorno il numero righe 
	DB->n_row = id_where;
	return true;
}

//ORDER BY desc vale true quando è desc; desc vale false quando è asc
bool selectORDERby(char*order_by, int desc, table_DB*DB) {
	//dichiarazione
	int last, i;
	char**aux;//serve per invertire la tabella
	bool errore;

	//inizializazione
	i = 0;
	last = DB->n_row-1;
	
	//ordina
	errore = sortDB(DB, order_by);
	if (errore == false)
		return false;
	
	if (desc == OP_DESC)//inverti tabella
	{
		while (last>i)
		{
			aux = DB->data[i];
			DB->data[i] = DB->data[last];
			DB->data[last] = aux;
			i++;
			last--;
		}
	}
	return true;

}

//GROUP BY
int* selectGROUPby(char*group_by, table_DB*DB){//modifica la tabella ragruppando per colonna, restituisce un vettore di interi lungo n_row del DB oppure NULL in caso di errore
	//dichiarazione
	int id_group, i,count_group,id_columns,*vet;
	bool errore;

	//inizializazione
	i = 0;//indice che scorre tutto il DB
	id_group = 0;//che si posiziona dove si può sovrascrivere il prossimo elemento
	count_group = 1;//contatore delle righe che hanno la colonna uguale
	id_columns = srcCOLUMNS(DB->columns, group_by, DB->n_columns);
	if (id_columns == -1)
		return NULL;
	vet = (int*)malloc(DB->n_row * sizeof(int));
	if (vet == NULL)
		return NULL;

	//ordina
	errore = sortDB(DB, group_by);
	if (errore == false)
		return false;

	//group
	while (i<DB->n_row){
		if (strcmp(DB->data[i][id_columns], DB->data[i + 1][id_columns]) == 0) {
			count_group++;//conto le righe che hanno le colonne uguali
			i++;//vado avanti con le righe fino a quando sono uguali
		}
		else{//se sono diversi raggruppo
			vet[id_group] = count_group;//carico il numero di righe uguali nel vettore
			DB->data[id_group] = DB->data[i];//carico la riga nella prima disponibile 
			id_group++;//incremento l'indice in cui andrò a scrivere/sovrascrivere al prossimo raggruppo
			count_group = 1;//inizializzo di nuovo il conteggio
		}
	}
	DB->n_row = count_group;//aggiorno il nuovo numero righe
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
	int  i,group_by;
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