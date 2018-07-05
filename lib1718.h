#ifndef LIB1718_H
#define LIB1718_H

#define bool int
#define true 1
#define false 0

/*
Input: 
	-La stringa contenente la query da eseguire, ad esempio "CREATE TABLE studenti (matricola, nome, cognome)"
Output:
	-bool: true/false, se l'esecuzione della query e' andata a buon fine o meno (presenza di eventuali errori) 
*/
bool executeQuery(char*); 

#endif //LIB1718_H
