#include <stdio.h>
#include <unistd.h>
int main()
{
	fprintf(stdout, "READY %d\n",(int)getpid());
	fflush(stdout);
	for(;;) pause();
	return 0;
}

