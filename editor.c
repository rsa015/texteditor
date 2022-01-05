#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>


#define QUASAR_VERSION "1.0.0"
#define ABUF_INIT {NULL, 0}
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
	ARROW_DOWN, 
	ARROW_UP,
	ARROW_LEFT = 1000,
	END_KEY,
	HOME_KEY,
	DEL_KEY,
	ARROW_RIGHT,
	PAGE_UP,
	PAGE_DOWN,
};

struct editorConfig {
	int cx, cy;
	int screenrows;	
	int screencols;	
	struct termios orig_termios;
};

struct abuf {
	char *b;
	int len;
};


struct editorConfig E;

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	exit(1);
}

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new =  realloc(ab->b, ab->len + len);

	if (new == NULL) return;

	memcpy(&new[ab->len], s, len);
	ab -> b;
	ab -> len += len;
}

void abFree(struct abuf *ab) {
	free(ab -> b);
}


int editorReadKey() {
	int nread;
	char c;
	while((nread = read(STDIN_FILENO, &c, 1) != 1)) {
		if (nread == -1 && errno != EAGAIN) die("read");	
	}

	if (c == '\x1b') {
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1)) return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}
		} else if (seq[0] == '0') {
			switch(seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}
		return '\x1b';
	} else {
		return c;
	}
}

int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;


	while (i < sizeof(buf)-1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}

	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

int getWindowsSize(int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
			return -1;	
		}
	
		return getCursorPosition(rows, cols);	
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		}
	return 0;	
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)== -1 || ws.ws_col == 0) {
		return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

void initEditor() {
	E.cx = 0;
	E.cy = 0;

	if (getWindowsSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}


void editorDrawsRows(struct abuf *ab) {
	int y;
	for (y =0; y < E.screenrows; y++) {
		if (y == E.screenrows / 3) {
			char welcome[80];
			int welcomelen = snprintf(welcome, sizeof(welcome), "Quasar --version %s", QUASAR_VERSION);
			if (welcomelen > E.screencols){welcomelen = E.screencols;};
			int padding = (E.screencols - welcomelen) / 2;
		
			if (padding) { abAppend(ab, "~", 1); padding--;}

			while (padding--) {abAppend(ab, " ", 1); }

			abAppend(ab, welcome, welcomelen);

		} else { abAppend(ab, "~", 1);}
	
		abAppend(ab, "\x1b[K", 3);	
		if (y < E.screenrows -1) {
		abAppend(ab, "\r\n", 2);
		}	
	}	

}


void editorMoveCursor(int key) {
	switch (key) {
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;	
			}
			break;
		case ARROW_RIGHT:
			if (E.cx != E.screencols - 1) {
				E.cx ++;
			}	
			break;
		case ARROW_DOWN:
			if (E.cy != 0) {
				E.cy--;
			}	
			break;
		case ARROW_UP:
			if (E.cy != E.screenrows) {
				E.cy++;
			}	
			break;
	}
}

void editorProcessKeypress() {
	int c = editorReadKey();

	switch(c) {
		case CTRL_KEY('q'):
			write(STDIN_FILENO, "\x1b[2J", 4);
			write(STDIN_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		case HOME_KEY:
			E.cx = 0;
			break;
		case END_KEY:
			E.cx = E.screencols - 1;
			break;
		case PAGE_UP:
		case PAGE_DOWN:
			{
				int times = E.screenrows;
				while (times--) {
					editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
				}
			}
			break;
		case ARROW_LEFT:
		case ARROW_RIGHT:
		case ARROW_DOWN:
		case ARROW_UP:
			editorMoveCursor(c);
		break;
	}	
}

void editorRefreshScreen() {
	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);

	editorDrawsRows(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cx + 1, E.cy + 1);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[H", 3);
	abAppend(&ab, "\x1b[?25h", 6);

	write(STDIN_FILENO, ab.b, ab.len);

	abFree(&ab);

}


void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcsetattr");
}


void enableRawMode() {
/*
Enables raw mode:
	 pass in the original value of struct at the beginning and in other to reset the terminal prperly at exit
*/

	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");

	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
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
	initEditor();	
	while (1) {
		editorRefreshScreen();	
		editorProcessKeypress();
	}	
}
