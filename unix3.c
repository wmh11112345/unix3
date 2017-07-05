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

static void
sig_int(int signo)
{
	printf("caught SIGINT\n");
}
static void
sig_chld(int signo)
{
	printf("caught SIGCHLD\n");
}

#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
ssize_t /* Read "n" bytes from a descriptor */
readn(int fd, void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return(-1); /* error, return -1 */
			else
				break; /* error, return amount read so far */
		}
		else if (nread == 0) {
			break; /* EOF */
		}
		nleft -= nread;
		ptr += nread;
	}
	return(n - nleft); /* return >= 0 */
}
ssize_t /* Write "n" bytes to a descriptor */
writen(int fd, const void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return(-1); /* error, return -1 */
			else
				break; /* error, return amount written so far */
		}
		else if (nwritten == 0) {
			break;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return(n - nleft); /* return >= 0 */
}

int
main(void)
{
	if (signal(SIGINT, sig_int) == SIG_ERR)
		err_sys("signal(SIGINT) error");
	if (signal(SIGCHLD, sig_chld) == SIG_ERR)
		err_sys("signal(SIGCHLD) error");
	if (system("/bin/ed") < 0)
		err_sys("system() error");
	exit(0);
}
