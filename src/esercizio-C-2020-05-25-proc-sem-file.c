/******* esercizio-C-2020-05-25-proc-sem-file ************/

/*
il processo principale crea un file "output.txt" di dimensione FILE_SIZE (all'inizio ogni byte del file deve avere valore 0)
```
#define FILE_SIZE (1024*1024)

#define N 4
```
è dato un semaforo senza nome: proc_sem

il processo principale crea N processi figli

i processi figli aspettano al semaforo proc_sem.

ogni volta che il processo i-mo riceve semaforo "verde", cerca il primo byte del file che abbia valore 0 e ci scrive il valore ('A' + i).
La scrittura su file è concorrente e quindi va gestita opportunamente (ad es. con un mutex).

se il processo i-mo non trova una posizione in cui poter scrivere il valore, allora termina.


il processo padre:

per (FILE_SIZE+N) volte, incrementa il semaforo proc_sem

aspetta i processi figli e poi termina.


risolvere il problema in due modi:

soluzione A:

usare le system call open(), lseek(), write()

soluzione B:

usare le system call open(), mmap()

```
int main() {
    printf("ora avvio la soluzione_A()...\n");
    soluzione_A();

    printf("ed ora avvio la soluzione_B()...\n");
    soluzione_B();

    printf("bye!\n");
    return 0;
}
```
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>

#define FILE_SIZE (1024*1024)

#define N 4

#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }
#define CHECK_ERR_MMAP(a,msg) {if ((a) == MAP_FAILED) { perror((msg)); exit(EXIT_FAILURE); } }

sem_t proc_sem;
sem_t mutex;

int create_file_set_size(char *file_name, unsigned int file_size) {

	int res;
	int fd = open(file_name,
	O_CREAT | O_RDWR,
	S_IRUSR | S_IWUSR);

	if (fd == -1) { // errore!
		perror("open()");
		return -1;
	}

	res = ftruncate(fd, 0);
	if (res == -1) {
		perror("ftruncate()");
		return -1;
	}

	res = ftruncate(fd, file_size);
	if (res == -1) {
		perror("ftruncate()");
		return -1;
	}

	return fd;
}

void write_in_file(int *fd, int i, off_t file_offset) {

	char text = 'A' + (char) i;

	if (lseek(*fd, file_offset, SEEK_SET) == -1) {
		perror("lseek");
		exit(EXIT_FAILURE);
	}

	// 3.4.2 Mutual exclusion solution, pag. 19
	if (sem_wait(&mutex) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}
	printf("Scrittura\n");
	// posizioniamo l'offset in fondo al file
	if ((lseek(*fd, file_offset, SEEK_SET)) == -1) {
		perror("lseek()");
		exit(EXIT_FAILURE);
	}

	int res = write(*fd, &text, sizeof(text));

	if (res == -1) {
		perror("write()");
		exit(EXIT_FAILURE);
	}
	if (sem_post(&mutex) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}
	return;
}

void soluzione_A(void) {
	// usare le system call open(), lseek(), write()
	char * file_name = "output_soluzione_A.txt";
	int fd;
	fd = create_file_set_size(file_name, FILE_SIZE);

	CHECK_ERR(fd, "create_file_set_size");

	int res;
	char *tmp;

	tmp = malloc(FILE_SIZE);

	for (int i = 0; i < N; i++) {

		switch (fork()) {
		case 0:

			if ((res = read(fd, tmp, FILE_SIZE)) == -1) {
				perror("read error");
				exit(EXIT_FAILURE);
			}

			for (int k = 0; k < FILE_SIZE; k++) {
				if (sem_wait(&proc_sem) == -1) {
					perror("sem_wait");
					exit(EXIT_FAILURE);
				}

				if (tmp[k] == 0)
					write_in_file(&fd, i, k);
				if (lseek(fd, 0, SEEK_CUR) == -1) {
					perror("lseek");
					exit(EXIT_FAILURE);
							}
			}
			exit(EXIT_SUCCESS);
		case -1:
			perror("fork()");
			exit(EXIT_FAILURE);
		default:
			for (int i = 0; i < FILE_SIZE + N; i++) {
				if (sem_post(&proc_sem) == -1) {
					perror("sem_post");
					exit(EXIT_FAILURE);
				}
			}

			if (wait(NULL) == -1) {
				perror("wait");
				exit(EXIT_FAILURE);
			}
		}
	}
	printf("Soluzione A completata, BYE!\n");
	return;
}


void soluzione_B(void) {
	// usare le system call open(), mmap()
}

int main() {

	int res = sem_init(&proc_sem,
					1, 	// 1 => il semaforo è condiviso tra processi, 0 => il semaforo è condiviso tra threads del processo
					1 	// valore iniziale del semaforo (se mettiamo 0 che succede?)
					);

	CHECK_ERR(res, "sem_init (proc_sem)")

	res = sem_init(&mutex,
					1, 	// 1 => il semaforo è condiviso tra processi, 0 => il semaforo è condiviso tra threads del processo
					1 	// valore iniziale del semaforo (se mettiamo 0 che succede?)
					);

	CHECK_ERR(res, "sem_init (mutex)")

    printf("ora avvio la soluzione_A()...\n");
    soluzione_A();

//    printf("ed ora avvio la soluzione_B()...\n");
//    soluzione_B();

    printf("bye!\n");
    return 0;
}
