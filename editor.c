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
	raw.c_oflag &= ~(OPOST);
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | ISTRIP | INPCK); //IXON:  Disable ctrl-S and ctrl-Q
	raw.c_cflag |= (CS8);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;	
	tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw); 
}


int main() {
	enableRawMode();
	char c = '\0';
	read(STDIN_FILENO, &c, 1);
	/*Reads the character in c, but exits as soon as it reads 'q'*/	
	while(1){
		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {	
			printf("%d ('%c')\r\n", c, c);
		}
		if (c = 'q') {break;}
	}
	return 0;
}
