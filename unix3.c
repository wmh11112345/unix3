#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <pwd.h>

#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

static jmp_buf env_alrm;
static void sig_quit(int);
#define err_sys perror
#define err_quit perror

#include <termios.h>
int
main(void)
{
	struct termios term;
	long vdisable;
	char a[10];
	if (isatty(STDIN_FILENO) == 0)
		err_quit("standard input is not a terminal device");
	if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0)
		err_quit("fpathconf error or _POSIX_VDISABLE not in effect");
	if (tcgetattr(STDIN_FILENO, &term) < 0) /* fetch tty state */
		err_sys("tcgetattr error");
	scanf("%s", a);
	term.c_cc[VINTR] = vdisable; /* disable INTR character */
	term.c_cc[VEOF] = 2; /* EOF is Control-B */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0)
		err_sys("tcsetattr error");
	scanf("%s", a);
	exit(0);
}