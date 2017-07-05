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

#include <fcntl.h>
#include <sys/mman.h>
#define COPYINCR (1024*1024*1024) /* 1 GB */
int
main(int argc, char *argv[])
{
	int fdin, fdout;
	void *src, *dst;
	size_t copysz;
	struct stat sbuf;
	off_t fsz = 0;
	if (argc != 3)
		err_sys("usage: %s <fromfile> <tofile>");
	if ((fdin = open(argv[1], O_RDONLY)) < 0)
		err_sys("can¡¯t open %s for reading");
	if ((fdout = open(argv[2], O_RDWR | O_CREAT | O_TRUNC,
		S_IWUSR|S_IWOTH|S_IWGRP)) < 0)
		err_sys("can¡¯t creat %s for writing");
	if (fstat(fdin, &sbuf) < 0) /* need size of input file */
		err_sys("fstat error");
	if (ftruncate(fdout, sbuf.st_size) < 0) /* set output file size */
		err_sys("ftruncate error");
	while (fsz < sbuf.st_size) {
		if ((sbuf.st_size - fsz) > COPYINCR)
			copysz = COPYINCR;
		else
			copysz = sbuf.st_size - fsz;
		if ((src = mmap(0, copysz, PROT_READ, MAP_SHARED,
			fdin, fsz)) == MAP_FAILED)
			err_sys("mmap error for input");
		if ((dst = mmap(0, copysz, PROT_READ | PROT_WRITE,
			MAP_SHARED, fdout, fsz)) == MAP_FAILED)
			err_sys("mmap error for output");
		memcpy(dst, src, copysz); /* does the file copy */
		msync(dst, copysz, MS_SYNC);
		munmap(src, copysz);
		munmap(dst, copysz);
		fsz += copysz;
	}
	exit(0);
}