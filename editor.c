#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


struct termios orig_termios;

void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}


void enableRawMode() {
	/*
	Enables raw mode:
		 pass in the original value of struct at the beginning and 		    in other to reset the terminal prperly at exit
	*/

	tcgetattr(STDIN_FILENO, &orig_termios);
	
	atexit(disableRawMode);

	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_iflag &= ~(IXON | ICRNL); //IXON:  Disable ctrl-S and ctrl-Q	
	tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw); 
}


int main() {
	enableRawMode();
	char c;
	/*Reads the character in c, but exits as soon as it reads 'q'*/	
	while(read(STDIN_FILENO, &c, 1)==1 && c != 'q'){
		if (iscntrl(c)) {
			printf("%d\n", c);
		} else {	
			printf("%d ('%c')\n", c, c);
		}
	}
	return 0;
}
