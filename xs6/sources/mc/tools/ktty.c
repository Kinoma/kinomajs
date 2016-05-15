/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.      
 *     You may obtain a copy of the License at
 *   
 *      http://www.apache.org/licenses/LICENSE-2.0                           
 *   
 *     Unless required by applicable law or agreed to in writing, software   
 *     distributed under the License is distributed on an "AS IS" BASIS,     
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and   
 *     limitations under the License.
 */  
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>

extern char **environ;

static int
open_port(const char *dev)
{
	int fd;
	int n;
	struct termios tty;
	
	if ((fd = open(dev, O_RDWR | O_NDELAY | O_NOCTTY, 0)) < 0) {
		perror(dev);
		return -1;
	}
	n = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, n & ~O_NDELAY);
	tcgetattr(fd, &tty);
	tty.c_iflag = IGNBRK;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cflag |= CLOCAL | CREAD;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 5;
	tty.c_iflag &= ~(IXON|IXOFF|IXANY);
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_cflag &= ~(PARENB | PARODD);
	tcsetattr(fd, TCSANOW, &tty);
	return fd;
}

int
main(int ac, char *av[])
{
	int fd;
	char **args;
	int i;

	if (ac < 3) {
		fprintf(stderr, "usage: %s tty cmd [args...]\n", av[0]);
		return -1;
	}
	ac -= 2;
	if ((args = malloc((ac + 1) * sizeof(char *))) == NULL)
		return -1;
	for (i = 0; i < ac + 1; i++)
		args[i] = av[2 + i];
	args[i] = NULL;
	if ((fd = open_port(av[1])) < 0)
		return -1;
	dup2(fd, 0);
	dup2(fd, 1);
	close(fd);
	execve(av[2], args, environ);
	perror(av[2]);
	return -1;
}
