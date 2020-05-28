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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define FILE_SIZE (1024*1024)
#define N 4

sem_t *proc_sem;
sem_t *mutex;

char * file_nameA = "output_A.txt";
char * file_nameB = "output_B.txt";

//int create_file_set_size(char *file_name, unsigned int file_size);
//void soluzione_A();
//void read_write_in_file(int fd, int i);

#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }
#define CHECK_ERR_MMAP(a,msg) {if ((a) == MAP_FAILED) { perror((msg)); exit(EXIT_FAILURE); } }

void create_file_set_size(char *file_name, unsigned int file_size) {
	int res;
	int fd = open(file_name,
	O_CREAT | O_TRUNC | O_RDWR, // apriamo il file in lettura e scrittura
	S_IRUSR | S_IWUSR // l'utente proprietario del file avrà i permessi di lettura e scrittura sul nuovo file
	);

	CHECK_ERR(fd, "open file")

	res = ftruncate(fd, file_size);
	CHECK_ERR(res, "ftruncate()")

	close(fd);
}

void read_write_in_file(int i) {

	int fd;
	int res;
	char ch, ch2write;
	off_t file_offset;
	int exit_while = 0;

	ch2write = 'A' + i;

	fd = open(file_nameA, O_RDWR);

	while (exit_while == 0) {

		if (sem_wait(proc_sem) == -1) {
			perror("sem_wait");
			exit(EXIT_FAILURE);
		}

		if (sem_wait(mutex) == -1) {
			perror("sem_wait");
			exit(EXIT_FAILURE);
		}

		while ((res = read(fd, &ch, 1)) > 0 && ch != 0);

		if (res == 1) {
			file_offset = lseek(fd, -1, SEEK_CUR);

			CHECK_ERR(file_offset, "lseek");

			printf("[child %d] scrivo all'offset %ld\n", i, file_offset);

			res = write(fd, &ch2write, sizeof(ch2write));
			CHECK_ERR(res, "Write error")
		} else {
			exit_while = 1;
		}

		if (sem_post(mutex) == -1) {
			perror("sem_post");
			exit(EXIT_FAILURE);
		}

	}
	close(fd);

	printf("[child %d] terminato bye!\n", i);
	exit(EXIT_SUCCESS);

}

void soluzione_A() {
//	usare le system call open(), lseek(), write()

	create_file_set_size("output_A.txt", FILE_SIZE);

	for (int i = 0; i < N; i++) {
		switch (fork()) {
		case 0:

			read_write_in_file(i);

			break;
		case -1:
			perror("fork()");
			exit(EXIT_FAILURE);
		default:
			;
		}
	}

	for (int i = 0; i < FILE_SIZE + N; i++) {
		if (sem_post(proc_sem) == -1) {
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
	}

	while (wait(NULL) != -1);

	printf("Soluzione A completata BYE!\n");
}

void write_mmap(char * mmap_addr, int i) {
	char ch2write = 'A' + (char) i;
	char * ptr = mmap_addr;

	int exit_while = 0;

	while (exit_while == 0) {
		if (sem_wait(proc_sem) == -1) {
			perror("sem_wait");
			exit(EXIT_FAILURE);
		}


		if (sem_wait(mutex) == -1) {
			perror("sem_wait");
			exit(EXIT_FAILURE);
		}

		while (*ptr != 0 && ptr - mmap_addr < FILE_SIZE)
			ptr++;

		if (ptr - mmap_addr == FILE_SIZE) {
			exit_while = 1;
		} else {
			printf("[child %d] scrivo all'offset %ld\n", i, ptr-mmap_addr);
			*ptr = ch2write;
			ptr++;
		}

		if (sem_post(mutex) == -1) {
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
	}

	printf("[child %d] terminato bye!\n", i);
	exit(EXIT_SUCCESS);


}

void soluzione_B() {
//	usare le system call open(), mmap()

	char * file_mmap;
	create_file_set_size(file_nameB, FILE_SIZE);

	int fd = open(file_nameB, O_RDWR);

	file_mmap = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			FILE_SIZE, // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED, // memory map condivisibile con altri processi
			fd, // agganciamento al fd del file
			0); // offset nel file

	CHECK_ERR_MMAP(file_mmap, "file mapping error");
	close(fd);
	// opzionale
	//memset(file_mmap, 0x00, FILE_SIZE);

	for (int i = 0; i < N; i++) {

		switch (fork()) {
		case 0:

			write_mmap(file_mmap, i);

			break;

		case -1:
			perror("fork()");
			exit(EXIT_FAILURE);
		default:
			;
		}
	}

	for (int i = 0; i < FILE_SIZE + N; i++) {
		if (sem_post(proc_sem) == -1) {
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
	}

	while (wait(NULL) != -1);

	printf("Soluzione B completata BYE!\n");
}





/******************************************************************************************/
int main(int argc, char *argv[]) {

	int res;

	proc_sem = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(sem_t) * 2, // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0); // offset nel file

	CHECK_ERR_MMAP(proc_sem, "mmap")

	mutex = proc_sem + 1;

	res = sem_init(proc_sem,
						1, // 1 => il semaforo è condiviso tra processi, 0 => il semaforo è condiviso tra threads del processo
						0 // valore iniziale del semaforo
						);
	CHECK_ERR(res, "Sem init")

	res = sem_init(mutex,
						1, // 1 => il semaforo è condiviso tra processi, 0 => il semaforo è condiviso tra threads del processo
						1 // valore iniziale del semaforo (se mettiamo 0 che succede?)
						);
	CHECK_ERR(res, "Mutex init")

	printf("ora avvio la soluzione_A()...\n");
	soluzione_A();

	printf("ed ora avvio la soluzione_B()...\n");
	soluzione_B();

	printf("bye!\n");
	return 0;
}
