#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <pwd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

static jmp_buf env_alrm;
static void sig_quit(int);
#define err_sys printf
#define err_quit printf

#include <errno.h>
#include <fcntl.h>
#if defined(SOLARIS)
#include <stropts.h>
#endif

int ptym_open(char *pts_name, int pts_namesz)
{
	char *ptr;
	int fdm, err;
	if ((fdm = posix_openpt(O_RDWR)) < 0)
		return(-1);
	if (grantpt(fdm) < 0) /* grant access to slave */
		goto errout;
	if (unlockpt(fdm) < 0) /* clear slave’s lock flag */
		goto errout;
	if ((ptr = ptsname(fdm)) == NULL) /* get slave’s name */
		goto errout;
	/*
	* Return name of slave. Null terminate to handle
	* case where strlen(ptr) > pts_namesz.
	*/
	strncpy(pts_name, ptr, pts_namesz);
	pts_name[pts_namesz - 1] = '\0';
	return(fdm); /* return fd of master */
errout:
	err = errno;
	close(fdm);
	errno = err;
	return(-1);
}
int
ptys_open(char *pts_name)
{
	int fds;
#if defined(SOLARIS)
	int err, setup;
#endif
	if ((fds = open(pts_name, O_RDWR)) < 0)
		return(-1);
#if defined(SOLARIS)
	/*
	* Check if stream is already set up by autopush facility.
	*/
	if ((setup = ioctl(fds, I_FIND, "ldterm")) < 0)
		goto errout;
	if (setup == 0) {
		if (ioctl(fds, I_PUSH, "ptem") < 0)
			goto errout;
		if (ioctl(fds, I_PUSH, "ldterm") < 0)
			goto errout; 726 Pseudo Terminals Chapter 19
		if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
		errout:
			err = errno;
			close(fds);
			errno = err;
			return(-1);
		}
	}
#endif
	return(fds);
}

pid_t
pty_fork(int *ptrfdm, char *slave_name, int slave_namesz,
const struct termios *slave_termios,
const struct winsize *slave_winsize)
{
	int fdm, fds; 
	pid_t pid;
	char pts_name[20];
	if ((fdm = ptym_open(pts_name, sizeof(pts_name))) < 0)
		err_sys("can’t open master pty: %s, error %d",pts_name,fdm);
	if (slave_name != NULL) 
	{
		/*
		* Return name of slave. Null terminate to handle case
		* where strlen(pts_name) > slave_namesz.
		*/
		strncpy(slave_name, pts_name, slave_namesz);
		slave_name[slave_namesz - 1] = '\0';
	}
	if ((pid = fork()) < 0)
	{
		return(-1);
	}
	else if (pid == 0)
	{ /* child */
		if (setsid() < 0)
			err_sys("setsid error");
		/*
		* System V acquires controlling terminal on open().
		*/
		if ((fds = ptys_open(pts_name)) < 0)
			err_sys("can’t open slave pty");
		close(fdm); /* all done with master in child */
#if defined(BSD)
		/*
		* TIOCSCTTY is the BSD way to acquire a controlling terminal.
		*/
		if (ioctl(fds, TIOCSCTTY, (char *)0) < 0)
			err_sys("TIOCSCTTY error");
#endif
		/*
		* Set slave’s termios and window size.
		*/
		if (slave_termios != NULL)
		{
			if (tcsetattr(fds, TCSANOW, slave_termios) < 0)
				err_sys("tcsetattr error on slave pty");
		}
		if (slave_winsize != NULL)
		{
			if (ioctl(fds,TIOCSWINSZ,slave_winsize) < 0)
				err_sys("TIOCSWINSZ error on slave pty");
		}
		/*
		* Slave becomes stdin/stdout/stderr of child.
		*/
		if (dup2(fds, STDIN_FILENO) != STDIN_FILENO)
			err_sys("dup2 error to stdin"); 

		if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO)
			err_sys("dup2 error to stdout");
		if (dup2(fds, STDERR_FILENO) != STDERR_FILENO)
			err_sys("dup2 error to stderr");
		if (fds != STDIN_FILENO && fds != STDOUT_FILENO &&
			fds != STDERR_FILENO)
			close(fds);
		return(0); /* child returns 0 just like fork() */
	}
	else 
	{ /* parent */
		*ptrfdm = fdm; /* return fd of master */
		return(pid); /* parent returns pid of child */
	}
}
#ifdef LINUX
#define OPTSTR "+d:einv"
#else
#define OPTSTR "d:einv"
#endif
static void set_noecho(int); /* at the end of this file */
#define BUFFSIZE 512
static void sig_term(int);
static volatile sig_atomic_t sigcaught; /* set by signal handler */
typedef void Sigfunc(int);
Sigfunc *signal_intr(int signo, Sigfunc *func)
{
	struct sigaction act, oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
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
void loop(int ptym, int ignoreeof)
{
	pid_t child;
	int nread;
	char buf[BUFFSIZE];
	if ((child = fork()) < 0) {
		err_sys("fork error");
	}
	else if (child == 0) { /* child copies stdin to ptym */
		for (;;) {
			if ((nread = read(STDIN_FILENO, buf, BUFFSIZE)) < 0)
				err_sys("read error from stdin");
			else if (nread == 0)
				break; /* EOF on stdin means we’re done */
			if (writen(ptym, buf, nread) != nread)
				err_sys("writen error to master pty");
		}
		/*
		* We always terminate when we encounter an EOF on stdin,
		* but we notify the parent only if ignoreeof is 0.
		*/
		if (ignoreeof == 0)
			kill(getppid(), SIGTERM); /* notify parent */
		exit(0); /* and terminate; child can’t return */
	}
	/*
	* Parent copies ptym to stdout.
	*/
	if (signal_intr(SIGTERM, sig_term) == SIG_ERR)
		err_sys("signal_intr error for SIGTERM");
	for (;;) {
		if ((nread = read(ptym, buf, BUFFSIZE)) <= 0)
			break; /* signal caught, error, or EOF */
		if (writen(STDOUT_FILENO, buf, nread) != nread)
			err_sys("writen error to stdout");
	}
	/*
	* There are three ways to get here: sig_term() below caught the
	* SIGTERM from the child, we read an EOF on the pty master (which
	* means we have to signal the child to stop), or an error.
	*/
	if (sigcaught == 0) /* tell child if it didn’t send us the signal */
		kill(child, SIGTERM);
	/*
	* Parent returns to caller.
	*/
}
/*
* The child sends us SIGTERM when it gets EOF on the pty slave or
* when read() fails. We probably interrupted the read() of ptym.
*/
static void
sig_term(int signo)
{
	sigcaught = 1; /* just set flag and return */
}
#include <sys/socket.h>
/*
* Returns a full-duplex pipe (a UNIX domain socket) with
* the two file descriptors returned in fd[0] and fd[1].
*/
int
fd_pipe(int fd[2])
{
	return(socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
}
static int ttysavefd = -1;
void do_driver(char *driver)
{
	pid_t child;
	int pipe[2];
	/*
	* Create a full-duplex pipe to communicate with the driver.
	*/
	if (fd_pipe(pipe) < 0)
		err_sys("can’t create stream pipe");
	if ((child = fork()) < 0) {
		err_sys("fork error");
	}
	else if (child == 0) { /* child */
		close(pipe[1]);
		/* stdin for driver */
		if (dup2(pipe[0], STDIN_FILENO) != STDIN_FILENO)
			err_sys("dup2 error to stdin");
		/* stdout for driver */
		if (dup2(pipe[0], STDOUT_FILENO) != STDOUT_FILENO)
			err_sys("dup2 error to stdout");
		if (pipe[0] != STDIN_FILENO && pipe[0] != STDOUT_FILENO)
			close(pipe[0]);
		/* leave stderr for driver alone */
		execlp(driver, driver, (char *)0);
		err_sys("execlp error for: %s", driver);
	}
	close(pipe[0]); /* parent */
	if (dup2(pipe[1], STDIN_FILENO) != STDIN_FILENO)
		err_sys("dup2 error to stdin");
	if (dup2(pipe[1], STDOUT_FILENO) != STDOUT_FILENO)
		err_sys("dup2 error to stdout");
	if (pipe[1] != STDIN_FILENO && pipe[1] != STDOUT_FILENO)
		close(pipe[1]);
	/*
	* Parent returns, but with stdin and stdout connected
	* to the driver.
	*/
}
void tty_atexit(void) /* can be set up by atexit(tty_atexit) */
{
	if (ttysavefd >= 0)
		tty_reset(ttysavefd);
}
static struct termios save_termios;

static enum { RESET, RAW, CBREAK } ttystate = RESET;
int
tty_cbreak(int fd) /* put terminal into a cbreak mode */
{
	int err;
	struct termios buf;
	if (ttystate != RESET) {
		errno = EINVAL;
		return(-1);
	}
	if (tcgetattr(fd, &buf) < 0)
		return(-1);
	save_termios = buf; /* structure copy */
	/*
	* Echo off, canonical mode off.
	*/
	buf.c_lflag &= ~(ECHO | ICANON);
	/*
	* Case B: 1 byte at a time, no timer.
	*/
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
		return(-1);
	/*
	* Verify that the changes stuck. tcsetattr can return 0 on
	* partial success.
	*/
	if (tcgetattr(fd, &buf) < 0) {
		err = errno;
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = err;
		return(-1);
	}
	if ((buf.c_lflag & (ECHO | ICANON)) || buf.c_cc[VMIN] != 1 ||
		buf.c_cc[VTIME] != 0) {/*
* Only some of the changes were made. Restore the
* original settings.
*/
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = EINVAL;
		return(-1);
	}
	ttystate = CBREAK;
	ttysavefd = fd;
	return(0);
}
int
tty_raw(int fd) /* put terminal into a raw mode */
{
	int err;
	struct termios buf;
	if (ttystate != RESET) {
		errno = EINVAL;
		return(-1);
	}
	if (tcgetattr(fd, &buf) < 0)
		return(-1);
	save_termios = buf; /* structure copy */
	/*
	* Echo off, canonical mode off, extended input
	* processing off, signal chars off.
	*/
	buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/*
	* No SIGINT on BREAK, CR-to-NL off, input parity
	* check off, don’t strip 8th bit on input, output
	* flow control off.
	*/
	buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	/*
	* Clear size bits, parity checking off.
	*/
	buf.c_cflag &= ~(CSIZE | PARENB);
	/*
	* Set 8 bits/char.
	*/
	buf.c_cflag |= CS8;
	/** Output processing off.
*/
	buf.c_oflag &= ~(OPOST);
	/*
	* Case B: 1 byte at a time, no timer.
	*/
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
		return(-1);
	/*
	* Verify that the changes stuck. tcsetattr can return 0 on
	* partial success.
	*/
	if (tcgetattr(fd, &buf) < 0) {
		err = errno;
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = err;
		return(-1);
	}
	if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
		(buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON)) ||
		(buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 ||
		(buf.c_oflag & OPOST) || buf.c_cc[VMIN] != 1 ||
		buf.c_cc[VTIME] != 0) {
		/*
		* Only some of the changes were made. Restore the
		* original settings.
		*/
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = EINVAL;
		return(-1);
	}
	ttystate = RAW;
	ttysavefd = fd;
	return(0);
}
int
tty_reset(int fd) /* restore terminal’s mode */
{
	if (ttystate == RESET)
		return(0);
	if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
		return(-1);
	ttystate = RESET;
	return(0);
}

struct termios *
	tty_termios(void) /* let caller see original tty state */
{
		return(&save_termios);
	}
int main(int argc, char *argv[])
{
	int fdm, c, ignoreeof, interactive, noecho, verbose;
	pid_t pid;
	char *driver;
	char slave_name[20]; 
	struct termios orig_termios;
	struct winsize size;
	interactive = isatty(STDIN_FILENO);
	ignoreeof = 0;
	noecho = 0;
	verbose = 0;
	driver = NULL;
	opterr = 0; /* don’t want getopt() writing to stderr */
	while ((c = getopt(argc, argv, OPTSTR)) != EOF) {
		switch (c) {
		case 'd': /* driver for stdin/stdout */
			driver = optarg;
			break;
		case 'e': /* noecho for slave pty’s line discipline */
			noecho = 1;
			break;
		case 'i': /* ignore EOF on standard input */
			ignoreeof = 1;
			break;
		case 'n': /* not interactive */
			interactive = 0;
			break;
		case 'v': /* verbose */
			verbose = 1;
			break;
		case '?':
			err_quit("unrecognized option: -%c", optopt);
		}
	}
	if (optind >= argc)
		err_quit("usage: pty [ -d driver -einv ] program [ arg ... ]");
	if (interactive) { /* fetch current termios and window size */
		if (tcgetattr(STDIN_FILENO, &orig_termios) < 0)
			err_sys("tcgetattr error on stdin");
		if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&size) < 0)
			err_sys("TIOCGWINSZ error");
		pid = pty_fork(&fdm, slave_name, sizeof(slave_name),
			&orig_termios, &size);
	}
	else {
		pid = pty_fork(&fdm, slave_name, sizeof(slave_name),
			NULL, NULL);
	}
	if (pid < 0) {
		err_sys("fork error");
	}
	else if (pid == 0) { /* child */
		if (noecho)
			set_noecho(STDIN_FILENO); /* stdin is slave pty */
		if (execvp(argv[optind], &argv[optind]) < 0)
			err_sys("can’t execute: %s", argv[optind]);
	}
	if (verbose) {
		fprintf(stderr, "slave name = %s\n", slave_name);
		if (driver != NULL)
			fprintf(stderr, "driver = %s\n", driver);
	}
	if (interactive && driver == NULL) {
		if (tty_raw(STDIN_FILENO) < 0) /* user’s tty to raw mode */
			err_sys("tty_raw error");
		if (atexit(tty_atexit) < 0) /* reset user’s tty on exit */
			err_sys("atexit error");
	}
	if (driver)
		do_driver(driver); /* changes our stdin/stdout */
	loop(fdm, ignoreeof); /* copies stdin -> ptym, ptym -> stdout */
	exit(0);
}
static void set_noecho(int fd) /* turn off echo (for slave pty) */
{
	struct termios stermios;
	if (tcgetattr(fd, &stermios) < 0)
		err_sys("tcgetattr error");
	stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	//stermios.c_lflag &= (˜(ECHO | ECHOE | ECHOK | ECHONL));
	/*
	* Also turn off NL to CR/NL mapping on output.
	*/
	stermios.c_oflag &= ~(ONLCR);
	if (tcsetattr(fd, TCSANOW, &stermios) < 0)
		err_sys("tcsetattr error");
}