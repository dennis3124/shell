/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <regex.h>
#include <termios.h>
#define MAX_BUFFER_LINE 2048

// Buffer where line is stored
int line_length;;
char line_buffer[MAX_BUFFER_LINE];
char charArray[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int upFlag = 0;
int downFlag = 0;
int history_index = 0;
extern char** history;
extern int maxHistory;
extern int historyCount;
int hisCounter = 0;
int history_length = sizeof(history)/sizeof(char *);
int leftCount = 0;
int rightCount = 0;
void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();
  
  line_length = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32) {
      // It is a printable character. 
	
	  int left = leftCount;
	  	if ( left != 0) {
		//left was typed 
		// now insert at that point
			char* charArray = (char*)malloc(sizeof(char) * left);
			int i = 0;
			while(i < left) {	
				charArray[i] = line_buffer[line_length - (left - i)];
				i++;
			} //end while loop
			//line_length now at insertion cursor
			// do what normal print does

	     // Do echo
		  line_length-=left;

	      write(1,&ch,1);

	      // If max number of character reached return.
	      if (line_length==MAX_BUFFER_LINE-2) break; 
	
	      // add char to buffer.
	      line_buffer[line_length]=ch;
	      line_length++;
		
			i = 0 ;
			//write the char into the terminal
			while(i <left) {
				char s = charArray[i];
				line_buffer[line_length] = s;
				line_length++;
				write(1,&s,1);
				i++;
			}
			i = 0;
			//move the cursor back to the insertion point
			while( i < left) {
				ch = 8;
				write(1,&ch,1);
				i++;
			}
		}
	    else {
		 // Do echo
	      write(1,&ch,1);

	      // If max number of character reached return.
	      if (line_length==MAX_BUFFER_LINE-2) break; 
	
	      // add char to buffer.
	      line_buffer[line_length]=ch;
	      line_length++;	
		}
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      if(historyCount == maxHistory) {
	maxHistory*=2;
	history = (char**)realloc(history,sizeof(char*)*maxHistory);
      }
      char* s = strdup(line_buffer);
      int l = line_length;
      s[l] = '\0';
      upFlag = 0;
      downFlag = 0;
      history_index = 0;
      history[historyCount++] = strdup(s);
      leftCount = 0;
      rightCount = 0;
      // Print newline
      write(1,&ch,1);

      break;
    }
	else if (ch==9) {
     		// <Tab> was typed. Path Completion
		
		DIR * dir = opendir(".");
		char* reg = (char*)malloc(sizeof(char) * line_length + 10);
		char* word = (char*)malloc(sizeof(char) * line_length);
		//printf("length is %d\n",line_length);
		int wordLength = line_length;
		int wordCount = 0;
		int j = 0;
		int i = 0;
		for(i = wordLength; i >= 0; i--) {
			if(line_buffer[i] == ' ') {
				break;
			}
		}
		
		
		if(i >= 0) {
			i++;
			j = i;
		}
		else {
			j = line_length;
			i++;
			//printf("%d\n",j);

		}
		*word = '^';
		word++;
		wordCount++;
		while(i != line_length) {
			*word = line_buffer[i];
			word++;
			wordCount++;
			i++;
			//write(1,&s,1);
			
		}
		*word = '\0';
		word = word - wordCount;
		
		//printf("word is %s\n",word);
		int maxEntry = 20;
		int entry = 0;
		
		
		regex_t re;
		int res = regcomp(&re,word,REG_NOSUB|REG_EXTENDED);

		if ( res != 0) {
			break;
		}

		
		char* array[maxEntry];
		char* temp;
      		struct dirent* ent;
		while((ent = readdir(dir))!= NULL) {
			regmatch_t match;
			if (regexec(&re,ent->d_name,1,&match,0) == 0) {	
				array[entry++] = strdup(ent->d_name);
			}
		}
		closedir(dir);
		if ( entry > 1) {
			int i;
			printf("\n");
			for(i = 0; i < entry; i++) {
				printf("%s\n",array[i]);
			}
		}
		else if ( entry == 1) {
			char* zz = strdup(array[0]);
			int i;
			//printf("%d\n",j);
			for(i = 0; i < j; i++) {
				char c = 8;
				write(1,&c,1);
				line_length--;
			}
			for(i = 0; i < strlen(zz); i++) {
				char c = zz[i];
				write(1,&c,1);
				line_buffer[line_length++] = c;
			}
			
		}
		
		//free(array);
	}	

    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8) {
      // <backspace> was typed. Remove previous character read.
		//if left was clicked
		// Go back one character
	 if (line_length >0) {
		int left = leftCount;
		if (left > 0) {
			char* charArray = (char*)malloc(sizeof(char) * left);
			int i =0;
			while ( i < left) {
				charArray[i] = line_buffer[line_length - (left - i)];
				i++;
			}
			line_length-=left;
			line_length--;
			ch = 8;
     	 		write(1,&ch,1);

			i = 0;
			while(i < left) {
				char s = charArray[i];
				line_buffer[line_length++] = s;
				write(1,&s,1);
				i++;
			}
			ch = ' ';
	 		write(1,&ch,1);

			i = 0;
			//move the cursor back to the insertion point
			while( i < left) {
				ch = 8;
				write(1,&ch,1);
				i++;
			}
			ch = 8;
			write(1,&ch,1);
		}
		else {
     	 		ch = 8;
     	 		write(1,&ch,1);
	
	      		// Write a space to erase the last character read
	      		ch = ' ';
	 		write(1,&ch,1);
	
	      		// Go back one character
	      		ch = 8;
	      		write(1,&ch,1);
	
	     		 // Remove one character from buffer
	     		 line_length--;
		}
	}
		
    }
	else if (ch == 5) {
		int left = leftCount;
		if ( leftCount != 0) {
			int i =0;
			while (i < left) {
				char s = line_buffer[line_length - (left - i)];
				write(1,&s,1);
				leftCount--;	
				i++;
			}
			
		}
		
		leftCount = 0;
	}

    	else if (ch == 1) {
		//Ctrl -A (HOME)
		int left = line_length - leftCount;
			int i =0;
			while(i < left) {
				ch = 8;
				write(1,&ch,1);
				i++;					
			}
			leftCount = line_length;
		
	}
	else if (ch == 4) {
		//ctrl-D (DELETE)
		if (line_length >0) {
		int left = leftCount;
			if (left > 0) {
				char* charArray = (char*)malloc(sizeof(char) * left);
				int i =0;
				while ( i < left -1) {
					charArray[i] = line_buffer[line_length - (left - i -1)];
					//char s = charArray[i];
					//write(1,&s,1);
					i++;
				}
				line_length-=(left-1);	
				line_length--;	
				i = 0;
				while(i < left-1) {
					char s = charArray[i];
					line_buffer[line_length++] = s;
					write(1,&s,1);
					i++;
				}
				ch = ' ';
	 			write(1,&ch,1);
	
				i = 0;
				//move the cursor back to the insertion point
				while( i < left-1) {
					ch = 8;
					write(1,&ch,1);
					i++;
				}
				ch = 8;
				write(1,&ch,1);	
				leftCount--;
			}
		}
		
	}

    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
	if ( historyCount >0){
		// Up arrow. Print next line in history.
		// Erase old line
		// Print backspaces
		upFlag = 1;
		int i = 0;
		for (i =0; i < line_length; i++) {
		  ch = 8;
		  write(1,&ch,1);
		}
	
		// Print spaces on top
		for (i =0; i < line_length; i++) {
		  ch = ' ';
		  write(1,&ch,1);
		}
	
		// Print backspaces
		for (i =0; i < line_length; i++) {
		  ch = 8;
		  write(1,&ch,1);
		}	
		if ( downFlag ) {
			history_index--;
			downFlag =0;
		}
		// Copy line from history
		history_index--;
		if (history_index < 0) {
			history_index = abs(history_index);
			history_index = historyCount - history_index;
		}
		strcpy(line_buffer, history[history_index]);
		line_length = strlen(line_buffer);
		//history_index=(history_index+1)%historyCount;
	
		// echo line
		write(1, line_buffer, line_length);
		}
      }
	else  if (ch1==91 && ch2==66) {
	if ( historyCount >0){
		// Down arrow. Print next line in history.
		// Erase old line
		// Print backspaces
		int i = 0;
		downFlag = 1;
		for (i =0; i < line_length; i++) {
		  ch = 8;
		  write(1,&ch,1);
		}
	
		// Print spaces on top
		for (i =0; i < line_length; i++) {
		  ch = ' ';
		  write(1,&ch,1);
		}
	
		// Print backspaces
		for (i =0; i < line_length; i++) {
		  ch = 8;
		  write(1,&ch,1);
		}	
		if(upFlag) {
			history_index=(history_index+1)%historyCount;
			upFlag = 0;
		}
		// Copy line from history
		strcpy(line_buffer, history[history_index]);
		line_length = strlen(line_buffer);
		history_index=(history_index+1)%historyCount;
	
		// echo line
		write(1, line_buffer, line_length);
		}
      }
	else if (ch1==91 && ch2==68) {
	  if(line_length != 0) {
	  	ch = 8;
   	  	write(1,&ch,1);
		leftCount++;
	  }
	}
	else if (ch1==91 && ch2==67) {
	  if(rightCount != line_length) {
		if (leftCount != 0) {
			int right = line_length - leftCount;
			leftCount--;
			char s = line_buffer[right];
			write(1,&s,1);
			
		}
	  }
	} 
	else  if (ch1==91 && ch2==49) {
		char ch3;
		read(0,&ch3,1);
		if (ch3 == 126) {
		//home key was pressed
			int left = line_length - leftCount;
			int i =0;
			while(i < left) {
				ch = 8;
				write(1,&ch,1);
				i++;					
			}
			leftCount = line_length;
		}

	}

	else  if (ch1==91 && ch2==52) {
		char ch3;
		read(0,&ch3,1);
		if (ch3 == 126) {
		//End key was pressed

		int left = leftCount;
		if ( leftCount != 0) {
			int i =0;
			while (i < left) {
				char s = line_buffer[line_length - (left - i)];
				write(1,&s,1);
				leftCount--;	
				i++;
			}
			
		}
		
		leftCount = 0;
		}

	}
      
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;
  line_length = 0;
  return line_buffer;
}

