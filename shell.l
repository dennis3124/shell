
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include "y.tab.h"
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
int maxHistory = 5;
char** history = (char**)malloc(sizeof(char*) * maxHistory);
int historyCount =0 ;
extern "C" char * read_line();
int mygetc(FILE * f) {
static char *p;
char ch;
if (!isatty(0)) {
  // stdin is not a tty. Call real getc
  
  return getc(f);
}
// stdin is a tty. Call our read_line.
if (p==NULL || *p == 0) {
  char * s = read_line();
  p = s;
}
ch = *p;
p++;
return ch;
}
#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );
int count = 0;
void myunputc(int c) {
    unput(c);
}


%}

%%

<<EOF>> {
		return EFILE;
	}

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

"|" 	{
		return PIPE;
	}


"&"	{
		return AMPERSAND;
	}

">&" 	{
		return GREATAMPERSAND;
	}

">>&" 	{
		return GREATGREATAMPERSAND;
	}

">>"	{
		return GREATGREAT;
	}

">" 	{
		return GREAT;
	}


"<"	{
		return LESS;
	}


[\`][^\`\n][^\`\n]*[\`] 	{	
			int tempin = dup(0);
			int tempout = dup(1);
			//create pipes
			int fdpipe[2];
			pipe(fdpipe);
			char buffer;
			


			//adjust yytext to remove backticks

			char* word = strdup(yytext);
			char* temp = (char*)malloc(strlen(word) * sizeof(char*));
			int count = 0;
			while (*word) {
				if (*word == '`') {
				 	word++;
				}
				else {
					*temp = *word;
					temp++;
					count++;
					word++;
				}
			}
			*temp = '\n';
			temp++;
			count++;
			*temp = '\0';
			temp -= count;
			//redirect yytext to child
			write(fdpipe[1], temp, strlen(temp));
			close(fdpipe[1]);
			
			
			//create 2nd pipe
			int fdpipetwo[2];
			pipe(fdpipetwo);

			//redirect std out to second pipe
			dup2(fdpipetwo[1],1);
			close(fdpipetwo[1]);


			//redirect input of child to fdpipe[0] 
			dup2(fdpipe[0],0);
			close(fdpipe[0]);

			int pid = fork(); //fork the process
			if (pid == 0) {
				char* arg[2];
				strcpy(arg[0],"/proc/self/exe");
				arg[1] = NULL;
				execvp(arg[0],arg);
				perror(arg[0]);
				exit(1);
				
				//child process
				//read from stdin
			} else if (pid < 1) {
				perror("fork");
				exit(1);
			}
			
			//now output is in fdpipetwo[1];
			//read from fdpipetwo[0];
				
			waitpid(0,NULL,0);
			dup2(tempout,1);
			close(tempout);

			char c;
			char* buf = (char*)malloc(1024 * sizeof(char*));
			int maxSize = 1024;
			int count2 = 0;

               while (read(fdpipetwo[0], &c, 1) > 0) {
				if(count2 == maxSize) {
					buf = (char*)realloc(buf,(size_t)maxSize * 2);
					maxSize *=2;
				}
				if (c != '\n' && c!= '\t') {
					*buf = c;
					//printf("buf is %c\n", c);

					buf++;
					count2++;
				} else {
					*buf = ' ';
					buf++;
					count2++;
				}
			}

			close(fdpipetwo[0]);
			dup2(tempin,0);
			close(tempin);

			*buf = '\0';
			buf = buf-count2;
			int length = strlen(buf);
			int index = length-2;
			for (index; index >= 0 ; index--) {
				myunputc(buf[index]);
			}		
		}

[a-zA-Z0-9]*[\\][^a-zA-Z0-9][^\n\t]* {
		char* word = strdup(yytext);
		char* temp = (char*)malloc(strlen(word) * sizeof(char*));
		int count = 0;
		int count2 = 0;
		while (*word) {
			if (*word == 92 && count2 < 1) {
			 	word++;
				count2++;
			}
			else {
				*temp = *word;
				temp++;
				count++;
				word++;
				count2=0;
			}
		}
		*temp = '\0';
		temp -= count;
		printf("word is = %s\n",temp);
		yylval.string_val = strdup(temp);
		return WORD;
	
	}

\"[^\n\"]*\" {
		char* word = strdup(yytext);
		char* temp = (char*)malloc(strlen(word) * sizeof(char*));
		int count = 0;
		while (*word) {
			if (*word == '"') {
			 	word++;
			}
			else {
				*temp = *word;
				temp++;
				count++;
				word++;
			}
		}
		*temp = '\0';
		temp -= count;
		yylval.string_val = strdup(temp);
		return WORD;
}


[^ \`\\><\&\|\t\n][^ \`\\><\&\|\t\n]*  {
		/* Assume that file names have only alpha chars */
		//printf("yytext is %s\n",yytext);
		yylval.string_val = strdup(yytext);
		return WORD;
}

