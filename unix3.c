#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <pwd.h>
static void sig_usr(int); /* one handler for both signals */
static void
my_alarm(int signo)
{
	struct passwd *rootptr;
	printf("in signal handler\n");
	if ((rootptr = getpwnam("root")) == NULL)
		perror("getpwnam(root) error\n");
	alarm(1);
}
int
main(void)
{
	struct passwd *ptr; 
	signal(SIGALRM, my_alarm);
	alarm(1);
	for (;;) {
		if ((ptr = getpwnam("sar")) == NULL)
			perror("getpwnam error\n");
		if (strcmp(ptr->pw_name, "sar") != 0)
			printf("return value corrupted!, pw_name = %s\n",
			ptr->pw_name);
	}
}