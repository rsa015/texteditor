#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

struct termios orig_termios;

void die(const char *s) {
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcsetattr");
}


void enableRawMode() {
	/*
	Enables raw mode:
		 pass in the original value of struct at the beginning and 		    in other to reset the terminal prperly at exit
	*/

	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
	
	atexit(disableRawMode);

	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_oflag &= ~(OPOST);
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | ISTRIP | INPCK); //IXON:  Disable ctrl-S and ctrl-Q
	raw.c_cflag |= (CS8);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw)== -1) die("tcsetattr"); 
}


int main() {
	enableRawMode();
	char c = '\0';	
	if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
	/*Reads the character in c, but exits as soon as it reads 'q'*/
	while(1) {
		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {	
			printf("%d ('%c')\r\n", c, c);
		}
		if (c = 'q') break;
	}
	return 0;
}
