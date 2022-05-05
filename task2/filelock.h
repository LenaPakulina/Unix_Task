#ifndef FILELOCK_H
#define FILELOCK_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOCK_POSTFIX ".lck"

int seed = -1;

typedef struct LF_FILE_s {
	FILE* p;
	char* name;
} LF_FILE;

const char* lf_lockname(const char* filename) {
	char* ret = strdup(filename);
	strcat(ret, LOCK_POSTFIX);
	return ret;
}

LF_FILE lf_open(const char* filename, const char* mode) {
	const char* lockname = lf_lockname(filename);

	LF_FILE ret;
	ret.p = NULL;
	ret.name = strdup(filename);

	int lock_d;
	wait_lock:
	while (access(lockname, F_OK) == 0) {} // Wait for the file to get removed

	if ((lock_d = open(lockname, O_CREAT|O_EXCL|O_WRONLY)) == -1) {
		if (errno == EEXIST) goto wait_lock; // Another process must've captured the lock first
		perror("Error opening lock file");
		return ret;
	}

	pid_t pid = getpid();
	write(lock_d, &pid, sizeof(pid_t));

	if (close(lock_d) != 0) {
		perror("Error closing lock file");
		return ret;
	}

	ret.p = fopen(filename, mode);

	return ret;
}

int lf_close(LF_FILE* who) {
	if (who->p == NULL) return 0;

	const char* lockname = lf_lockname(who->name);

	int lock_d = open(lockname, O_RDONLY);
	if (lock_d == -1) {
		perror("Error opening lock file");
		return -1;
	}

	pid_t readpid;
	read(lock_d, &readpid, sizeof(pid_t));

	if (readpid != getpid()) {
		fprintf(stderr, "Lock file contains PID %d, my PID: %d\n", readpid, getpid());
		return -1;
	}

	if (close(lock_d) != 0) {
		perror("Error closing lock file");
		return -1;
	}

	if (fclose(who->p) != 0) {
		perror("Could not close file");
		return -1;
	}

	who->p = NULL;
	free(who->name);

	if (remove(lockname) != 0) {
		perror("Could not remove lock file");
		return -1;
	}

	return 0;
}

#endif
