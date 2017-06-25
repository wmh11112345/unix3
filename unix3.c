#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <pwd.h>

#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

static jmp_buf env_alrm;
static void sig_quit(int);
void err_sys(const char* string)
{
	printf(string);
}
int
main(void)
{
	sigset_t newmask, oldmask, pendmask;
	if (signal(SIGQUIT, sig_quit) == SIG_ERR)
		err_sys("can¡¯t catch SIGQUIT");
	/*
	* Block SIGQUIT and save current signal mask.
	*/
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGQUIT);
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
		err_sys("SIG_BLOCK error");
	sleep(5); /* SIGQUIT here will remain pending */
	if (sigpending(&pendmask) < 0)
		err_sys("sigpending error");
	if (sigismember(&pendmask, SIGQUIT))
		printf("\nSIGQUIT pending\n");
	/*
	* Restore signal mask which unblocks SIGQUIT.
	*/
	if (sigprocmask(SIG_UNBLOCK, &newmask, NULL) < 0)
		err_sys("SIG_SETMASK error");
	printf("SIGQUIT unblocked\n");
	sleep(5); /* SIGQUIT here will terminate with core file */
	exit(0);
}
static void
sig_quit(int signo)
{
	printf("caught SIGQUIT\n");
	if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
		err_sys("can¡¯t reset SIGQUIT");
}