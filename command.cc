
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include "command.h"
extern "C" void read_line();
extern void myunputc(int c);
extern int mygetc(FILE* f);
int returnCode = -1;
pid_t currentPid;
pid_t lastPid;
pid_t pid;
int maxPid = 5;
pid_t * childPid = (pid_t *) malloc ( sizeof(pid_t) * maxPid);
int pidCount =0;

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	//printf("ARGUMENT IS : %s\n",argument);
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}
	if ( argument[0] == '~') {	
		if ( strlen(argument) == 1) {
			uid_t uid = geteuid();
			passwd* pass;
			pass = getpwuid(uid);
			argument = strdup(pass->pw_dir);
		}
		else {
			//copy evedrything after ~
			int count = 0;
			char* r = argument;
			char*word = (char*)malloc(strlen(argument) * sizeof(char*));
			r++;
			while (*r) {
				if (*r != '/') {
					*word = *r;
					r++;
					word++;
					count++;
				}
				else {	
					break;
				}
			}
			*word = '\0';
			word -= count;
			passwd* pass;
			pass = getpwnam(word);
			if (pass != NULL) {
				char* final = (char*)malloc(sizeof(char*) * 1024);
				final = strdup(pass->pw_dir);
				char* temp = strchr(argument, '/');
				if (temp != NULL) {	
					strcat(final,temp);
					argument = strdup(final);
				} else {
					argument = strdup(final);
				}
			}
			
		}
			
	}
	const char* reg = "^.*[$][{][^ }][^ }]*[}].*$";
	regex_t re;
	int result = regcomp(&re, reg, REG_NOSUB|REG_EXTENDED);
	regmatch_t match;
	
	//char buffer[100];
	//regerror(result,&re,buffer,100);
	//printf("%s\n",buffer);
	if (result == 0) {
		if (regexec(&re,argument,1,&match,0)==0) {
			char* word = strdup(argument);
			char* temp;
			char* temp2 = (char*)malloc(3 * strlen(word) * sizeof(char*));
			char* temp3 = temp2;
			int count = 0;
			char* s;
			//printf("%s\n", word);
			while(*word) {
				if (*word == '$') {
					temp = (char*)malloc(strlen(word) * sizeof(char*));
					count = 0;
					word++;
					if (*word == '{') {
						word++; 
						while(*word != '}' && *word) {
							*temp = *word;
							temp++;
							word++;
							count++;
						  }
						*temp = '\0';
						temp -=count;
						char* env;
						if(!strcmp(temp,"$")) {
							int pid = getpid();
						 	env = (char*) malloc(sizeof(char) * 255);
							sprintf(env,"%d",pid);
						}
						else if(!strcmp(temp,"?")) {
							printf("test");
						}
						else if(!strcmp(temp,"SHELL")) {
							env = getenv("_");
						} 
						else if (!strcmp(temp,"_")) {
							env = getenv("lastCommand");
							
						}
						else if (!strcmp(temp,"!")) {
							//printf("test");
						 	env = (char*) malloc(sizeof(char) * 255);
							sprintf(env,"%d",lastPid);
						}	
						else {
							env = getenv(temp);
						}
						if (env != NULL) {
							//printf("env = %s\n",env);
							*temp2='\0';
							strcat(temp3,env);
							temp2+=strlen(env);
							

						}
						word++;
					}
				}
				else {
					*temp2 = *word;
					temp2++;
					word++;	
				}
			}
			*temp2 = '\0';
			
			_arguments[ _numOfArguments ] = strdup(temp3);
			
			// Add NULL argument at the end
			_arguments[ _numOfArguments + 1] = NULL;
		
			_numOfArguments++;
			
		}
		else {
			_arguments[ _numOfArguments ] = argument;
	
			// Add NULL argument at the end
			_arguments[ _numOfArguments + 1] = NULL;
	
			_numOfArguments++;
		}
	}
	else {
		perror("Compilation error\n");
	}
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_iomodified=false;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	//print();
	// Don't do anything if there are no simple commands

	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Save default in, out and err
	
	int defaultin = dup(0);
	int defaultout = dup(1);
	int defaulterr = dup(2);
	int fderror;

	// use initial infile, outfile and err
	int fdin;
	

	if (_inFile) {
		fdin = open(_inFile, O_RDONLY);
	}
	else {
		fdin = dup(defaultin);
	}
	
	// Print contents of Command data structure
	//print();
	
	int ret;
	int fdout;
	

	for(int i = 0; i < _numOfSimpleCommands; i++) {
		//redirect input
		dup2(fdin, 0);
		close(fdin);
		
		//setup output
		if ( i == _numOfSimpleCommands-1) {
			if(_outFile) {
				if(_append) {
					fdout = open(_outFile, O_CREAT|O_WRONLY|O_APPEND,0664);
				}
				else {
					fdout = open(_outFile,O_CREAT|O_WRONLY|O_TRUNC,0664);
				}
			} else {
				fdout = dup(defaultout);
			}
			if(_errFile) {
				if (_append) {
					fderror = open(_outFile, O_CREAT|O_WRONLY|O_APPEND,0664);
				}
				else {
					fderror = open(_outFile,O_CREAT|O_WRONLY|O_TRUNC,0664);
				}
					dup2(fderror,2);
					close(fderror);
			}
			else {
					fderror = dup(defaulterr);
			}
		} else {
			//create pipe
			int fpipe[2];
			pipe(fpipe);
			fdout = fpipe[1];
			fdin = fpipe[0];
		}
		dup2(fdout,1);
		close(fdout);
		
		ret = fork();

		if (ret == 0) {
			if (!strcmp(_simpleCommands[i]->_arguments[0],"printenv")) {
				char**p = environ;
				while(*p!=NULL) {
					printf("%s\n",*p);
					p++;
				}
				exit(0);		
			}

			if(!strcmp(_simpleCommands[i]->_arguments[0], "source")|| !strcmp(_simpleCommands[i]->_arguments[0], "bg")||!strcmp(_simpleCommands[i]->_arguments[0], "fg")||!strcmp(_simpleCommands[i]->_arguments[0],"setenv") || !strcmp(_simpleCommands[i]->_arguments[0],"unsetenv") || !strcmp(_simpleCommands[i]->_arguments[0],"cd") || !strcmp(_simpleCommands[i]->_arguments[0],"jobs")) {
				exit(0);
			}
			
			//child process
			returnCode = execvp(_simpleCommands[i]->_arguments[0],_simpleCommands[i]->_arguments);
			perror(_simpleCommands[i]->_arguments[0]);
			exit(1);
		}
		else if (ret < 0 ) {
			//error
			perror("fork");
			return;
		}

		else {
			//parent process
			int res = setenv("lastCommand", _simpleCommands[i]->_arguments[0],1);
			
			if (pidCount == maxPid) {
				maxPid *=2;
				childPid = (pid_t *) realloc ( childPid , sizeof(pid_t) * maxPid);
			}
			else {
				childPid[pidCount++] = ret;
			}	
			
			if(!strcmp(_simpleCommands[i]->_arguments[0],"setenv")) {
				int res = setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2],1);
			}
	
			else if(!strcmp(_simpleCommands[i]->_arguments[0],"unsetenv")) {
				int res = unsetenv(_simpleCommands[i]->_arguments[1]);
			}
		
			else if(!strcmp(_simpleCommands[i]->_arguments[0],"cd")) {
					char* home = getenv("HOME");
				if (_simpleCommands[i]->_numOfArguments == 1) {
					int res = chdir(home);
				}
				else if(!strcmp(_simpleCommands[i]->_arguments[1],"~")) {
					int res = chdir(home);
				}
				else {
					int res = chdir(_simpleCommands[i]->_arguments[1]);
					if (res) {
					 perror("chdir");
					}
				}		
			}
		
			else if (!strcmp(_simpleCommands[i]->_arguments[0],"jobs")) {
				if (_simpleCommands[i]->_numOfArguments > 1) {
					if (!strcmp(_simpleCommands[i]->_arguments[1],"-p")) {
						for(int i = 0; i < pidCount; i++) {
							printf("%d\n", childPid[i]);
						}	
					}
				}
				else {
					for(int i = 0; i < pidCount; i++) {
						char str[50] = "ps -cO -p ";
    						char ppid [7];
    						sprintf(ppid,"%d", childPid[i]);
    						strcat(str,ppid);
    						system(str);
					}
				}
			}

			else if (!strcmp(_simpleCommands[i]->_arguments[0],"fg")) {
				if (_simpleCommands[i]->_numOfArguments != 1) {
					int childId = atoi(_simpleCommands[i]->_arguments[1]);
					waitpid(childId,NULL,0);
				}
			}

			else if (!strcmp(_simpleCommands[i]->_arguments[0],"bg")) {
				if (_simpleCommands[i]->_numOfArguments != 1) {
					int childId = atoi(_simpleCommands[i]->_arguments[1]);
					int res;
					if ((res = kill(childId, SIGCONT)) != 0) {
						perror("bg");
					}
				}

			}
			else if (!strcmp(_simpleCommands[i]->_arguments[0], "source")) {
				if(_simpleCommands[i]->_numOfArguments == 2) {
					char* fName = _simpleCommands[i]->_arguments[1];
					FILE * file = fopen(fName, "r");
					int fdin = open(fName, O_RDONLY);
					//save default in
					int defIn = dup(0);
					if ( file != NULL) {
						dup2(fdin, 0);
						char c;
						char* word =(char*) malloc(sizeof(char) * 1024);
						int wCount = 0;
						char* temp = word;
						while((c = mygetc(file)) > 0) {
							*temp = c;
							temp++;
							wCount++;

						}
						for(int i = wCount; i >= 0; i--) {
							myunputc(word[i]);
						}

						dup2(defIn,0);
						close(defIn);
					}
					else {
						printf("File does not exist\n");

					}
				}
				 else if (_simpleCommands[i]->_numOfArguments == 1){
					printf("File name argument required\n");
				}
			}
		}
		
	}

	dup2(defaultin,0);
	dup2(defaultout,1);
	close(defaultout);
	close(defaultin);
	
	currentPid = ret;
	if(!_background){
		//wait for child process to finish
		waitpid(ret,NULL,0);
		returnCode = 1;
	}else {
		lastPid = ret;
	} 

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	if ( isatty(0) ) {
		char* prompt = getenv("PROMPT");

		if ( prompt == NULL) {
			printf("myshell>");
			fflush(stdout);
		}
		else {
			printf("%s ",prompt);
			fflush(stdout);
		}
	}
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;
int yyparse(void);

extern void disp( int sig )
{
	if (sig == SIGINT) {
		printf("\n");
		Command::_currentCommand.prompt();
		fflush(stdout);
	}
	else if( sig == SIGCHLD) {
		//int status;
		int res;
		childPid[pidCount] = 0;
		pidCount--;
		while((res = waitpid(-1, NULL, WNOHANG)) > 0){};
		
	}
	else if( sig == SIGTSTP) {
		if(currentPid > 0) {
			kill(currentPid,SIGKILL);
		}
		else {
			printf("\n");
			Command::_currentCommand.prompt();
		}
	}
};

main()
{
    struct sigaction sa;
    sa.sa_handler = disp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &sa, NULL)){
        perror("sigaction");
        exit(2);
    }
 
    struct sigaction sa2;
    sa2.sa_handler = disp;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD, &sa2, NULL)){
   	 perror("sigaction");
   	 exit(2);
    }

    struct sigaction sa3;
    sa3.sa_handler = disp;
    sigemptyset(&sa3.sa_mask);
    sa3.sa_flags = SA_RESTART;

    if(sigaction(SIGTSTP, &sa3, NULL)){
        perror("sigaction");
        exit(2);
    }
	
	
	Command::_currentCommand.prompt();
	yyparse();
	
}

