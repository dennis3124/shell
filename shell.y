
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	EFILE NOTOKEN GREAT NEWLINE GREATGREAT PIPE AMPERSAND GREATGREATAMPERSAND GREATAMPERSAND LESS

%union	{
		char   *string_val;
	}

%{
#define yylex yylex
#define MAXFILENAME 1024
#include <assert.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include "command.h"
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
void yyerror(const char * s);
void sortArrayStrings(char*** array, int nEntries);
void expandWildcardsIfNecessary(char* arg);
void expandWildcards(const char* prefix, char* suffix);
void checkSubDir(char* arg);
char* toRegex(char* arg);
int yylex();
int n = 0;
int max = 20;
char** items;

%}

%%

goal: command_list;

arg_list:
	arg_list WORD {
		//	Command::_currentSimpleCommand->insertArgument($2);
		//	printf("test2: %s\n", $2);
			checkSubDir($2);
		//	expandWildcardsIfNecessary($2);	
	}
	| 
	;

cmd_and_args:
	WORD {
             //  printf("   Yacc: insert command \"%s\"\n", $1);
		if(!strcmp($1,"exit")) {
				printf("\n Good bye!!\n\n");
				exit(0);
		}
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	 arg_list {
		Command::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

pipe_list:
	pipe_list PIPE cmd_and_args
	| cmd_and_args
	;

	
io_modifier:
	GREATGREAT WORD {
		//printf("   Yacc: insert output append \"%s\"\n", $2);
		Command::_currentCommand._background=1;
		Command::_currentCommand._outFile = strdup($2);
		Command::_currentCommand._append = 1;
	}
	| GREAT WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		if (!Command::_currentCommand._iomodified) {
			Command::_currentCommand._iomodified=true;
		}
		else {
			yyerror("Ambiguous output redirect.\n");
			Command::_currentCommand._iomodified=false;


		}
	}
	| GREATGREATAMPERSAND WORD {
		//printf("   Yacc: insert output stderr append \"%s\"\n", $2);
		Command::_currentCommand._background=1;
		Command::_currentCommand._errFile=strdup($2);
		Command::_currentCommand._outFile =strdup($2);
		Command::_currentCommand._append = 1;
	}
	| GREATAMPERSAND WORD {
		//printf("   Yacc: insert output stderr \"%s\"\n", $2);
		Command::_currentCommand._errFile= strdup($2);
		Command::_currentCommand._outFile = strdup($2);
	}
	| LESS WORD {
		//printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inFile = $2;
	}
	;

io_modifier_list:
	io_modifier_list io_modifier
	|
	;

background_optional:
	AMPERSAND {
		 Command::_currentCommand._background = 1;
	 }
	|
	;

command_line:
	pipe_list io_modifier_list
		background_optional NEWLINE {
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	|
	pipe_list io_modifier_list
		background_optional EFILE {
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	|	NEWLINE 
	|	EFILE {
		//printf("   Yacc: Execute command\n");
		exit(1);
		}
	|	error NEWLINE{yyerrok;}

command_list:
	command_line |
	command_list command_line
	;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}


void
expandWildcardsIfNecessary(char* arg) 
{
	//Returns if arg does not contain '*' or '?'
	if (strchr(arg, '*')== NULL && strchr(arg, '?') == NULL){
		Command::_currentSimpleCommand->insertArgument(arg);
		//printf("TEST IS : %s\n",arg);
		return;
	}
	else {
		char* reg = toRegex(arg);
		//printf("%s\n",reg);
		regex_t re;

		int result = regcomp(&re, reg, REG_NOSUB|REG_EXTENDED);
		if (result!= 0){
			perror("compile");
			return;
		}

		DIR * dir = opendir(".");
		if (dir == NULL) {
			perror("opendir");
			return;
		}
		struct dirent * ent;
		int maxEntries = 20;
		int nEntries = 0;
		char** array = (char**) malloc(maxEntries * sizeof(char*));
		while ((ent = readdir(dir))!= NULL) {
		//check if name matches
			regmatch_t match;
			if(regexec(&re,ent->d_name,1,&match,0)==0) {
				if(nEntries == maxEntries) {
					maxEntries *=2;
					array = (char**)realloc(array, maxEntries* sizeof(char*));
					assert(array!=NULL);
				}
				if(ent->d_name[0] == '.'){
					if(arg[0] == '.') {
					 array[nEntries++] = strdup(ent->d_name);
					}
				}
				else {
				array[nEntries++] = strdup(ent->d_name);
				}
			}
		}
		closedir(dir);
		sortArrayStrings(&array,nEntries);
		for(int i = 0; i < nEntries ; i++) {
			//printf("%s\n", array[i]);
			Command::_currentSimpleCommand->insertArgument(array[i]);
		}
		free(array);
	}
	
}

void
sortArrayStrings(char*** array1, int nEntries) {
	char** array = *array1;
	char* temp;
	for (int i = 0; i < nEntries; i++){
		for(int j = 0; j < nEntries; j++) {
			if ((strcmp(array[i],array[j])) < 0) {
				temp = strdup(array[i]);
				array[i] = strdup(array[j]);
				array[j] = strdup(temp);
				
			}
		}
	}
	free(temp);
}

void
checkSubDir(char* arg) {
	if ((strchr(arg,'/'))== NULL) {
		expandWildcardsIfNecessary(arg);
	}
	else {
		if (arg[0] == '/') {
			items = (char**) malloc(max * sizeof(char*));
			expandWildcards("",arg);
			sortArrayStrings(&items,n);
			for(int i = 0; i < n ; i++) {
				//printf("item is %s\n", items[i]);
				Command::_currentSimpleCommand->insertArgument(strdup(items[i]));
			}
			free(items);
			n = 0;
		}
		else {
			Command::_currentSimpleCommand->insertArgument(strdup(arg));
		}
	}
}

void
expandWildcards(const char* prefix, char* suffix) {
 	
	if(suffix[0] == 0) {
		//suffix is empty. Put prefix in argument.
		//printf("prefix is : %s\n", prefix);
		//check for double // start
		if (prefix[0] == '/' && prefix[1] == '/') {
			prefix++;
		}
		if ( n == max) {
			max *=2;
			items = (char**)realloc(items, max*sizeof(char*));
			assert(items!=NULL);
		}
		items[n++] = strdup(prefix);
		return;
	}
	//Obtain the next component in the suffix
	//Also, advance the suffix.
	char * s = strchr(suffix, '/');
	//printf("prefix is : %s\n", s);
	char component[MAXFILENAME];
	if(s!=NULL) {	//Copy up to the first "/"
		strncpy(component,suffix,s-suffix);
		//printf("component is %s\n", component);
		suffix = s + 1;
	}
	else { //Last part of the path. Copy whole thing.
		strncpy(component,suffix,strlen(suffix));
		suffix = suffix + strlen(suffix);

	}
	//Now expand the component

	char newPrefix[MAXFILENAME];
	if (strchr(component, '*')== NULL && strchr(component, '?') == NULL){
		//No wildcard found
		if(prefix[0] == '/' && strlen(prefix) == 1) {
			sprintf(newPrefix, "%s%s", prefix, component);

		}else if(prefix[0] == 0) {
			sprintf(newPrefix,"/%s",component);
		}
		else {
			sprintf(newPrefix, "%s/%s", prefix, component);
		}
		//printf("new prefix is : %s\n",newPrefix);
		expandWildcards(newPrefix,suffix);
		return;
	}

	

	//Component has wildcard
	//Change component to regExpression
	char* reg = toRegex(component);
	regex_t re;
	int result = regcomp(&re, reg, REG_NOSUB|REG_EXTENDED);
		if (result!= 0){
			perror("compile");
			return;
		}
	char* dir;
	//If prefix is empty then list current directory
	//else directory is the prefix

	if (prefix[0] == 0){
		*dir = '.';
	}
	else {
		dir = strdup(prefix);		
	}
	DIR * d = opendir(dir);
	if (d== NULL) {
		return;
	}
	struct dirent * ent;

	//Check what entries match
	while ((ent = readdir(d)) != NULL) {
	//check if name match
		regmatch_t match;
			if(regexec(&re,ent->d_name,1,&match,0)==0) {	
				//entry matches. Add name of entry to the prefix and call expandwilCard() recursively
				//printf("%s\n", ent->d_name);
				if (ent->d_name[0] == '.') {
					if(component[0] == '.') {
						sprintf(newPrefix,"%s/%s", prefix, ent->d_name);
						expandWildcards(newPrefix,suffix);
					}
				}
				else {
					sprintf(newPrefix,"%s/%s", prefix, ent->d_name);
					expandWildcards(newPrefix,suffix);
				}
		}
	}
	closedir(d);
	
}


char* toRegex(char* arg) {
	char* reg = (char*)malloc(2*strlen(arg)+10);
		char* a = arg;
		char * r = reg;
		*r = '^';
		r++; //match begining of line with ^
		while (*a) {
			if(*a == '*') {
				*r='.';
				r++;
				*r='*';
				r++;
			}
			else if(*a=='?'){
				*r='.';
				r++;
			}
			else if(*a=='.'){
				*r='\\';
				r++;
				*r='.';
				r++;
			}
			else {
				*r = *a;
				r++;
			}
			a++;
		}
		*r='$';
		r++;
		*r=0; //match end of line and add null char
		return reg;

}

#if 0
main()
{	
	yyparse();
}
#endif
