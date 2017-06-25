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
#include <errno.h>
void
pr_mask(const char *str)
{
	sigset_t sigset;
	int errno_save;
	errno_save = errno; /* we can be called by signal handlers */
	if (sigprocmask(SIG_BLOCK, NULL, &sigset) < 0) {
		err_sys("sigprocmask error");
	}
	else {
		printf("%s", str);
		if (sigismember(&sigset, SIGINT))
			printf(" SIGINT");
		if (sigismember(&sigset, SIGQUIT))
			printf(" SIGQUIT");
		if (sigismember(&sigset, SIGUSR1))
			printf(" SIGUSR1");
		if (sigismember(&sigset, SIGALRM))
			printf(" SIGALRM");
		/* remaining signals can go here */
		printf("\n");
	}
	errno = errno_save; /* restore errno */
}
static void sig_int(int);
int
main(void)
{
	sigset_t newmask, oldmask, waitmask;
	pr_mask("program start: ");
	if (signal(SIGINT, sig_int) == SIG_ERR)
		err_sys("signal(SIGINT) error");
	sigemptyset(&waitmask);
	sigaddset(&waitmask, SIGUSR1);
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGINT);
	/*
	* Block SIGINT and save current signal mask.
	*/
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
		err_sys("SIG_BLOCK error");
	/*
	* Critical region of code.
	*/
	printf("the proccess id is %d\n", getpid());
	pr_mask("in critical region: ");
	/*
	* Pause, allowing all signals except SIGUSR1.
	*/
	pr_mask("before the sigsuspend:");
	if (sigsuspend(&waitmask) != -1)
		err_sys("sigsuspend error");
	pr_mask("after return from sigsuspend: ");
	/*
	* Reset signal mask which unblocks SIGINT.
	*/
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
		err_sys("SIG_SETMASK error");
	/*
	* And continue processing ...
	*/
	pr_mask("program exit: ");
	exit(0);
}
static void
sig_int(int signo)
{
	pr_mask("\nin sig_int: ");
}