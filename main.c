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



// // Funktion zum Aufnehmen der Gabeln
// void pick_up_forks(int sem_id, int left, int right) {
//     // Struktur für Semaphoroperationen
//     struct sembuf sem_op;
//     // Atomare Operation, um beide Gabeln aufzunehmen
//     sem_op.sem_num = left;
//     sem_op.sem_op = -1; // Linke Gabel nehmen
//     sem_op.sem_flg = 0;
//     // semop(sem_id, &sem_op, 1);
//     if (semop(sem_id, &sem_op, 1) < 0) {
//         perror("Error in P() operation0");
//         exit(1);
//     }

//     sem_op.sem_num = right;
//     sem_op.sem_op = -1; // Rechte Gabel nehmen
//     // semop(sem_id, &sem_op, 1);
//     if (semop(sem_id, &sem_op, 1) < 0) {
//         perror("Error in P() operation1");
//         exit(1);
//     }
// }

void pick_up_forks(int sem_id, int left, int right) {
    struct sembuf ops[2];

    // Configure the first operation to lock the left fork
    ops[0].sem_num = left;   // Semaphore number for the left fork
    ops[0].sem_op = -1;      // Decrement by 1 (take the fork)
    ops[0].sem_flg = 0;      // No special flags

    // Configure the second operation to lock the right fork
    ops[1].sem_num = right;  // Semaphore number for the right fork
    ops[1].sem_op = -1;      // Decrement by 1 (take the fork)
    ops[1].sem_flg = 0;      // No special flags

    // Perform the atomic semaphore operation on both forks
    if (semop(sem_id, ops, 2) < 0) {
        perror("Error in atomic P() operation for picking up forks");
        exit(1);
    }
}




// // Funktion zum Ablegen der Gabeln
// void put_down_forks(int sem_id, int left, int right) {
//     // Struktur für Semaphoroperationen
//     struct sembuf sem_op;
//     sem_op.sem_num = left;
//     sem_op.sem_op = 1; // Linke Gabel ablegen
//     sem_op.sem_flg = 0;
//     // semop(sem_id, &sem_op, 1);
//     if (semop(sem_id, &sem_op, 1) < 0) {
//         perror("Error in P() operation2");
//         exit(1);
//     }

//     sem_op.sem_num = right;
//     sem_op.sem_op = 1; // Rechte Gabel ablegen
//     // semop(sem_id, &sem_op, 1);
//     if (semop(sem_id, &sem_op, 1) < 0) {
//         perror("Error in P() operation3");
//         exit(1);
//     }
// }

void put_down_forks(int sem_id, int left, int right) {
    struct sembuf ops[2];

    // Configure the first operation to release the left fork
    ops[0].sem_num = left;
    ops[0].sem_op = 1;       // Increment by 1 (release the fork)
    ops[0].sem_flg = 0;

    // Configure the second operation to release the right fork
    ops[1].sem_num = right;
    ops[1].sem_op = 1;       // Increment by 1 (release the fork)
    ops[1].sem_flg = 0;

    // Perform the atomic semaphore operation to release both forks
    if (semop(sem_id, ops, 2) < 0) {
        perror("Error in atomic V() operation for putting down forks");
        exit(1);
    }
}


// Prozessfunktion für Philosophen
void philosopher(int phil_id, int sem_id) {
    int left = phil_id;
    int right = (phil_id + 1) % NUM_PHILOSOPHERS;

    while (1) {
        // Philosoph denkt
        printf("Philosopher %d is thinking.\n", phil_id);
        sleep(rand() % 3); // Zufällige Denkzeit

        // Gabeln aufnehmen
        printf("Philosopher %d is hungry and tries to pick up forks.\n", phil_id);
        pick_up_forks(sem_id, left, right);

        // Philosoph isst
        printf("Philosopher %d is eating.\n", phil_id);
        sleep(rand() % 3); // Zufällige Essenszeit

        // Gabeln ablegen
        put_down_forks(sem_id, left, right);
        printf("Philosopher %d has finished eating and put down forks.\n", phil_id);
    }
}

int main() {
    int sem_id;
    pid_t pid;
    // int status;
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

    // Erstellen der Philosophenprozesse
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pid = fork();
        if (pid == 0) {
            // Kindprozess führt Philosophen-Aktion aus
            srand(getpid()); // Initialisieren des Zufallszahlengenerators
            philosopher(i, sem_id);
            exit(0);
        } else if (pid < 0) {
            perror("Fehler beim Erstellen des Prozesses");
            exit(1);
        }
    }

    // Wait for all philosopher processes to complete
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        wait(NULL);
    }

    // Löschen der Semaphoren
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Fehler beim Löschen der Semaphoren");
        exit(1);
    }

    return 0;
}
