# esercizio-C-2020-05-25-proc-sem-file


il processo principale crea un file "output.txt" di dimensione FILE_SIZE (all'inizio ogni byte del file deve avere valore 0)
```
#define FILE_SIZE (1024*1024)

#define N 4
```
è dato un semaforo senza nome: proc_sem

il processo principale crea N processi figli

i processi figli aspettano al semaforo proc_sem.

ogni volta che il processo i-mo riceve semaforo "verde", cerca il primo byte del file che abbia valore 0 e ci scrive il valore ('A' + i). La scrittura su file è concorrente e quindi va gestita opportunamente (ad es. con un mutex).

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
