#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filelock.h"

LF_FILE locked_file;
char* lockstat_file = "lockstat.txt";
int number = 0;
int locks = 0;

void sigint(int signal) {
	lf_close(&locked_file);
	printf("[%d]\tTermination by SIGINT\n", number);
	LF_FILE statf = lf_open(lockstat_file, "a");
	if (statf.p == NULL) {
		fprintf(stderr, "[%d]\tCouldn't open stat file!\n", number);
		exit(1);
	}

	fprintf(statf.p, "Task number %d (PID %d) had %d successful locks\n", number, getpid(), locks);

	if (lf_close(&statf) != 0) {
		fprintf(stderr, "[%d]\tCouldn't close stat file!\n", number);
		exit(1);
	}
	exit(0);
}

int main(int argc, char** argv) {
	signal(SIGINT, sigint);

	int optc;
	while ((optc = getopt(argc, argv, "n:S:")) != -1) switch (optc) {
		case 'h':
			printf("Usage: %s -n task_number\n", argv[0]);
			return 0;
		case 'n':
			number = atoi(optarg);
			break;
		case 'S':
			lockstat_file = strdup(optarg);
			break;
		case '?':
			switch (optopt) {
				case 'n':
					fprintf(stderr, "-n: number required\n");
					break;
				case 'S':
					fprintf(stderr, "-S: file name required\n");
					break;
				default:
					fprintf(stderr, "Option unrecognized: -%c\n", optopt);
					break;
			}
			return 1;
	}

	printf("[%d]\tTask 2\n", number);

	locks = 0;
	while (1) {
		locked_file = lf_open("test.txt", "w");
		if (locked_file.p == NULL) break; // Failure to open the file with end the program
		++locks;
		printf("[%d]\tWaiting 1 sec to release the lock...\n", number);
		sleep(1);
		if (lf_close(&locked_file) != 0) break; // Failure to release the lock will end the program
	}

	fprintf(stderr, "[%d]\tUnexpected termination!\n", number);
	LF_FILE statf = lf_open(lockstat_file, "a");
	if (statf.p == NULL) {
		fprintf(stderr, "[%d]\tCouldn't open stat file!\n", number);
		return 1;
	}

	fprintf(statf.p, "Task number %d (PID %d) failed after %d successful locks\n", number, getpid(), locks);

	if (lf_close(&statf) != 0) {
		fprintf(stderr, "[%d]\tCouldn't close stat file!\n", number);
		return 1;
	}

	return 1;
}
