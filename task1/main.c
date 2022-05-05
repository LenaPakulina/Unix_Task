#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define USAGE printf("Usage:\n\t%s [-b block_size] out_file - reads from stdin\n"\
		"\t%s [-b block_size] in_file out_file - reads from in_file\n", argv[0], argv[0]);

int infile = -1;
int outfile = -1;

long block_size = 4096;

int main(int argc, char** argv) {
	int optc;

	while ((optc = getopt(argc, argv, "hb:")) != -1) switch (optc) {
		case 'h':
			USAGE;
			return 0;
			break;
		case 'b':
			block_size = atol(optarg);
			break;
		case '?':
			switch (optopt) {
				case 'b':
					fprintf(stderr, "-b: block size required\n");
					break;
				default:
					fprintf(stderr, "Option unrecognized: -%c\n", optopt);
					break;
			}
			return 1;
	}

	switch (argc - optind) {
		case 2:
			if ((infile = open(argv[optind], O_RDONLY)) == -1) {
				perror("Could not open input file");
				return 1;
			}

			if ((outfile = open(argv[optind + 1], O_CREAT|O_WRONLY, 0666)) == -1) {
				perror("Could not open output file");
				if (close(infile) != 0)
					perror("Could not close input file");
				return 1;
			}
			break;
		case 1:
			infile = STDIN_FILENO;

			if ((outfile = open(argv[optind], O_CREAT|O_WRONLY, 0666)) == -1) {
				perror("Could not open output file");
				return 1;
			}
			break;
		case 0:
			USAGE;
			return 1;
	}

	if (ftruncate(outfile, 0) == -1) {
		perror("ftruncate() failed");
		return 1;
	}

	long pos = 0;
	long zeros = 0;
	long block = 0;
	char c;
	char c_zero = 0;

	while (read(infile, &c, 1) != 0) {
		++pos;
		switch (c) {
			case 0:
				++zeros;
				break;
			default:
				while (zeros > 0) {
					if (write(outfile, &c_zero, 1) == -1) {
						perror("write() failed");
						return 1;
					}
					--zeros;
				}
				if (write(outfile, &c, 1) == -1) {
					perror("write() failed");
					return 1;
				}
				break;
		}

		if (pos % block_size == 0) {
			++block;
			if (lseek(outfile, block_size * block, SEEK_SET) == -1) {
				perror("lseek() failed");
				return 1;
			}
			pos = 0;
			zeros = 0;
		}

		/*
		 * This looks good in a terminal, but doesn't behave well with file output
		printf("\x1b[K\rByte %7ld, block %10ld (block_size %7ld)", pos, block, block_size);
		fflush(stdout);
		*/
	}

	printf("Copied %ld bytes\n", block * block_size + pos);

	if (infile != STDIN_FILENO) if (close(infile) != 0) {
		perror("Could not close input file");
		return 1;
	}

	if (close(outfile) != 0) {
		perror("Could not close output file");
		return 1;
	}

	return 0;
}
