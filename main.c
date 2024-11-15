#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define NUM_PHILOSOPHERS 5

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} argument;


void pick_up_forks(int sem_id, int left, int right) {
    struct sembuf ops[2];

    // left fork
    ops[0].sem_num = left;
    ops[0].sem_op = -1;      
    ops[0].sem_flg = 0;      

    // right fork
    ops[1].sem_num = right; 
    ops[1].sem_op = -1;
    ops[1].sem_flg = 0;

    if (semop(sem_id, ops, 2) < 0) {
        perror("Error in atomic P() operation for picking up forks");
        exit(1);
    }
}

void put_down_forks(int sem_id, int left, int right) {
    struct sembuf ops[2];

    // left fork
    ops[0].sem_num = left;
    ops[0].sem_op = 1;       
    ops[0].sem_flg = 0;

    // right fork
    ops[1].sem_num = right;
    ops[1].sem_op = 1;       
    ops[1].sem_flg = 0;

    if (semop(sem_id, ops, 2) < 0) {
        perror("Error in atomic V() operation for putting down forks");
        exit(1);
    }
}

int main() {
    int sem_id;
    pid_t pid;
    key_t sem_key;

    if ((sem_key = ftok("./semfile.txt", '1')) < 0) {
        perror("Error in ftok");
        exit(1);
    }


    if ((sem_id = semget(sem_key, NUM_PHILOSOPHERS, IPC_CREAT | 0666)) < 0) {
        perror("Error in semget");
        exit(1);
    }


    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        argument.val = 1;
        if (semctl(sem_id, i, SETVAL, argument) < 0) {
            perror("Fehler beim Initialisieren der Semaphoren");
            return -1;
        }
    }   

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        switch (fork()) {
            case -1:
                perror("Fork failed");
                exit(1);

            case 0:  // Child process
                srand(getpid()); // Initialisieren des Zufallszahlengenerators
                // philosopher(i, sem_id);
                int left = i;
                int right = (i + 1) % NUM_PHILOSOPHERS;

                while (1) {
                    // Philosoph denkt
                    printf("Philosopher %d is thinking.\n", i);
                    sleep(rand() % 3);

                    // Gabeln aufnehmen
                    printf("Philosopher %d is hungry and tries to pick up forks.\n", i);
                    pick_up_forks(sem_id, left, right);

                    // Philosoph isst
                    printf("Philosopher %d is eating.\n", i);
                    sleep(rand() % 3); 

                    // Gabeln ablegen
                    put_down_forks(sem_id, left, right);
                    printf("Philosopher %d has finished eating and put down forks.\n", i);
                }
                exit(0);
            default:
                break;
        }
    }

    // wait for philosopher
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        wait(NULL);
    }

    return 0;
}
