#include <fcntl.h>
#include <limits.h>	// PATH_MAX
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PROC 1024
#define MAX_ARGS 1024

char config_file[PATH_MAX + 1];
char log_file[PATH_MAX + 1];

typedef struct myinit_pinfo_s {
	char*	cmdline[MAX_ARGS];
	char*	p_stdout;
	char*	p_stdin;
} myinit_pinfo;

myinit_pinfo*	cplist[MAX_PROC];
pid_t		runlist[MAX_PROC];
long		num_procs;

int watching = 0;
int autorestart = 0;

// Close stdout (log file) and exit the program
void daemon_exit(int code) {
	for (long i = 0; i < MAX_PROC; ++i) {
		if (cplist[i] == NULL) continue;
		for (long j = 0; j < MAX_ARGS; ++j)
			if (cplist[i]->cmdline[j] == NULL) break;
			else free(cplist[i]->cmdline[j]);
		free(cplist[i]->p_stdin);
		free(cplist[i]->p_stdout);
		free(cplist[i]);
	}

	if (fclose(stdin) != 0) {
		perror("Failed to close the stdin file (it might've not been open in the main process, "
				"so it's ok to see this message once)");
		exit(1);
	}

	if (fclose(stdout) != 0) {
		perror("Failed to close the stdout file!");
		exit(1);
	}

	exit(code);
}

void fprint_pinfo(FILE* file, myinit_pinfo* info) {
	for (long i = 0; i < MAX_ARGS; ++i)
		if (info->cmdline[i] == NULL) break;
		else fprintf(file, "%s ", info->cmdline[i]);
	fprintf(file, "< %s > %s\n", info->p_stdin, info->p_stdout);
}

// Read config and run processes
int config_init(const char* config_filename) {
	printf("Config file %s...\n", config_filename);

	// Read config file
	FILE* config = fopen(config_filename, "r");
	if (config == NULL) {
		perror(config_filename);
		return 1;
	}

	printf("Config open, beginning read...\n");

	char c;
	long word_start = 0;
	long word_end = 0;

	char*	cp_args[MAX_ARGS] = {};
	char*	cp_stdout	= NULL;
	char*	cp_stdin	= NULL;

	long cp_num = 0;
	while ((c = fgetc(config)) != EOF)
		switch (c) {
			case ' ': case '\n':
				{

				word_start = word_end;
				word_end = ftell(config);

				// Go back to word start and read the word
				char word[word_end - word_start];
				fseek(config, word_start, SEEK_SET);
				fread(word, sizeof(char), word_end - word_start, config);
				word[word_end - word_start - 1] = '\0';

				// Push the word
				free(cp_args[0]);
				for (long i = 0; i < MAX_ARGS - 1; ++i) cp_args[i] = cp_args[i + 1];
				cp_args[MAX_ARGS - 1] = cp_stdin;
				cp_stdin = cp_stdout;
				cp_stdout = strdup(word);

				if (c == '\n') {
					// Shift down the args
					while (cp_args[0] == NULL) {
						for (long i = 0; i < MAX_ARGS - 1; ++i)
							cp_args[i] = cp_args[i + 1];
						cp_args[MAX_ARGS - 1] = NULL;
					}
					// Write the entry
					cplist[cp_num] = malloc(sizeof(myinit_pinfo));
					cplist[cp_num]->p_stdin = strdup(cp_stdin);
					cplist[cp_num]->p_stdout = strdup(cp_stdout);
					memmove(cplist[cp_num]->cmdline, cp_args, MAX_ARGS * sizeof(char*));
					runlist[cp_num] = -1;

					cp_stdin = NULL;
					cp_stdout = NULL;
					for (long i = 0; i < MAX_ARGS; ++i) cp_args[i] = NULL;

					++cp_num;
				} // if (c == '\n')

				}
				break;
			default:
				break;
		}

	printf("Found %ld required child process(es)\n", cp_num);
	num_procs = cp_num;

	for (long i = 0; i < num_procs; ++i)
		if (cplist[i] == NULL) break;
		else {
			printf("#%ld: ", i);
			fprint_pinfo(stdout, cplist[i]);
		}

	if (fclose(config) != 0) {
		perror("Failed to close config file");
		return 1;
	}

	return 0;
}

void run_task(long cp_i) {
	printf("Forking child process #%ld\n", cp_i);
	pid_t cp_pid = fork();

	switch (cp_pid) {
		case -1:
			perror("fork() failed");
			daemon_exit(1);
			break;
		case 0:
			// In child process
			printf("Preparing child process %ld: ", cp_i);
			fprint_pinfo(stdout, cplist[cp_i]);

			int in_fd = open(cplist[cp_i]->p_stdin, O_CREAT|O_RDONLY, 0666);
			if (in_fd == -1) {
				perror("Could not open process input file");
				daemon_exit(1);
			}
			int out_fd = open(cplist[cp_i]->p_stdout, O_CREAT|O_APPEND|O_WRONLY, 0666);
			if (out_fd == -1) {
				perror("Could not open process output file");
				daemon_exit(1);
			}

			if (dup2(in_fd, STDIN_FILENO) < 0) {
				perror("dup2 failed for stdin");
				daemon_exit(1);
			}
			if (in_fd != STDIN_FILENO) if (close(in_fd) == -1) {
				perror("Could not close in_fd");
				daemon_exit(1);
			}

			if (dup2(out_fd, STDOUT_FILENO) < 0) daemon_exit(1);
			if (dup2(out_fd, STDERR_FILENO) < 0) daemon_exit(1);

			if ((out_fd != STDOUT_FILENO) || (out_fd != STDERR_FILENO)) if (close(out_fd) == -1) {
				perror("Could not close out_fd");
				daemon_exit(1);
			}

			execv(cplist[cp_i]->cmdline[0], cplist[cp_i]->cmdline);
			daemon_exit(0);
			break;
		default:
			runlist[cp_i] = cp_pid;
			printf("Launched child process %ld with PID %d\n", cp_i, cp_pid);
			break;
	}
}

void restart(int signal) {
	if (signal != 0) {
		printf("Restarting with signal %d\n", signal);
		autorestart = 0;

		// Kill all the child processes
		for (long cp_i = 0; cp_i < num_procs; ++cp_i) if (runlist[cp_i] != -1) {
			printf("Killing child %ld (PID %d)...\n", cp_i, runlist[cp_i]);
			kill(runlist[cp_i], SIGTERM);
		}

		if (watching) watching = 0;
	}

	num_procs = 0;

	// Now run child procs
	switch (signal) {
		case SIGKILL: case SIGINT:
			printf("Goodbye!\n");
			daemon_exit(0);
			break;
		default:
			// Read the config
			if (config_init(config_file) != 0) daemon_exit(1);

			printf("Config read, ready to serve processes!\n");

			// Check for absolute paths
			for (long cp_i = 0; cp_i < num_procs; ++cp_i)
				if ((cplist[cp_i]->cmdline[0][0] != '/') ||
					(cplist[cp_i]->p_stdout[0] != '/') ||
					(cplist[cp_i]->p_stdin[0] != '/')) {
					printf("Config process name or IO file names contain relative paths!\n"
						"For entry %ld: ", cp_i);
					fprint_pinfo(stdout, cplist[cp_i]);
					printf("Aborting starup and exiting!\n");
					daemon_exit(1);
				}

			watching = 1;
			autorestart = 1;
			for (long cp_i = 0; cp_i < num_procs; ++cp_i) run_task(cp_i);
			break;
	}
}

int main(int argc, char** argv) {
	signal(SIGHUP, restart);
	signal(SIGINT, restart);
	signal(SIGKILL, restart);

	char* config_opt = "./myinit.conf";
	char* log_opt = "/tmp/myinit.log";

	int optc;
	while ((optc = getopt(argc, argv, "hc:l:")) != -1) switch (optc) {
		case 'h':
			printf("Usage: %s [-c config_file] [-l log_file]\n", argv[0]);
			return 0;
		case 'c':
			config_opt = strdup(optarg);
			break;
		case 'l':
			log_opt = strdup(optarg);
			break;
		case '?':
			if (optopt == 'c') fprintf(stderr, "Config file name required!\n");
			else fprintf(stderr, "Option unrecognized: -%c\n", optopt);
			return 1;
	}

	realpath(config_opt, config_file);
	realpath(log_opt, log_file);

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork() failed");
		return 1;
	}

	if (pid != 0) return 0;

	// Chdir to root
	chdir("/");

	// Open the log file
	int log_fd = open(log_file, O_CREAT|O_APPEND|O_WRONLY, 0666);
	if (log_fd == -1) {
		perror(log_file);
		return 1;
	}

	// Daemonize
	if (setsid() == -1) {
		perror("setsid() failed");
		daemon_exit(1);
	}

	// Close/redirect standart IO
	if (close(STDIN_FILENO) == -1) { perror("Could not close stdin"); return 1; }
	if (dup2(log_fd, STDOUT_FILENO) < 0) { close(log_fd); return 1; }
	if (dup2(log_fd, STDERR_FILENO) < 0) { close(log_fd); return 1; }
	if (close(log_fd) == -1) return 1;

	if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
		perror("Failed to turn off buffering for the log file");
		daemon_exit(1);
		// Turn off buffered output to avoid double-writing
		// to file when child process closes stdout
	}

	printf("--------------------------------------------------------------\n");
	printf("myinit daemonized and ready to run programs!\n");

	restart(0);

	pid_t cp_pid;
	while (watching) {
		cp_pid = waitpid(-1, NULL, 0);

		for (long cp_i = 0; cp_i < num_procs; ++cp_i) if (runlist[cp_i] == cp_pid) {
			printf("Child process %ld (PID %d) ended.\n", cp_i, cp_pid);
			runlist[cp_i] = -1;
			if (!autorestart) continue;
			printf("Restarting...\n");
			run_task(cp_i);
			break;
		}
	}

	daemon_exit(0);
}
