/* @Author

Student Name: Enes Demirag
Student ID  : 504201571
Mail        : ensdmrg@gmail.com
Date        : 09.05.2021

Dear sir/madam,

This program creates 2 child processes and they wait each other to write data onto a shared memory.
Below commands can be used to build and run this project with gcc.

>> gcc -Wall -Werror main.c -o main
>> ./main input.txt output.txt

If you encounter any problem regarding this program, feel free to reach me.
I tested this code on my Ubuntu 20.04 machine.

Thanks and kind regards,
Enes
*/

// Including necassary libraries.
// For shared memory and semaphores
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
// For handling signals
#include <signal.h>
// For fork
#include <unistd.h>
// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global keys for semaphores and shared memory.
int KEYSEM1;
int KEYSEM2;
int KEYSHM;

/* This function creates keys for semaphores and shared memory
 * that are usable in subsequent calls to semget() and shmget().
 */
void initKeys(char* argv[])
{
    char cwd[256];
    char* key_string;

    getcwd(cwd, 256); // Get current working directory.
    
    // Allocate keystring
    key_string = malloc(strlen(cwd) + strlen(argv[0]) + 1);
    strcpy(key_string, cwd);
    strcat(key_string, argv[0]);

    // ftok() function returns a key based on path and id that is usable in subsequent calls to semget() and shmget().
    KEYSEM1 = ftok(key_string, 1);
    KEYSEM2 = ftok(key_string, 2);
    KEYSHM  = ftok(key_string, 3);
    
    // Deallocate keystring
    free(key_string);
}

/* This function increments the semaphore with given value.
 * @param sem_id Semaphore ID.
 * @param val Value of incrementation.
 */
void semSignal(int sem_id, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;
    semaphore.sem_flg = 0;
    semop(sem_id, &semaphore, 1);
}

// Signal handler function
void mySignal(int sig_num)
{
    printf("Received signal with num = %d\n", sig_num);
}

/* This function decrements the semaphore with given value.
 * @param sem_id Semaphore ID.
 * @param val Value of decrementation.
 */
void semWait(int sem_id, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = -val;
    semaphore.sem_flg = 0;
    semop(sem_id, &semaphore, 1);
}

/* This function sets the signaling.
 * @param num Signal number.
 */
void sigSet(int num)
{
    struct sigaction my_sigaction;
    my_sigaction.sa_handler = (void*) mySignal;

    // Using the signal-catching function identified by sa_handler.
    my_sigaction.sa_flags = 0;

    // sigaction() system call is used to change the action taken by
    // a process on receipt of a specific signal(specified with num);
    sigaction(num, &my_sigaction, NULL);
}

/* Main function */
int main(int argc, char *argv[])
{
    sigSet(12);       // Signal handler with num=12
    int children[2];       // Child process ids
    int shm_id     = 0;    // Shared memory id
    int* globalcp  = NULL; // Shared memory area
    int child_sem  = 0;    // Semaphore between child 0 and child 1.
    int parent_sem = 0;    // Semaphore between the child 1 and parent.
    int i          = 0;    // Process number.
    int my_order   = 0;    // Order of the running process
    int M          = 0;    // M
    int n          = 0;    // n
    int counter    = 0;    // For keeping the memory address.
    char space[]   = " ";  // Space character for splitting.

    // Initialize semaphore and shared memory keys
    initKeys(argv);

    // Read from file and fill n, M values and A array.
    FILE*  input_file;
    char*  line1 = NULL;
    char*  line2 = NULL;
    char*  line3 = NULL;
    size_t len   = 0;

    input_file = fopen(argv[1], "r");

    // Read three lines.
    getline(&line1, &len, input_file);
    getline(&line2, &len, input_file);
    getline(&line3, &len, input_file);

    // Set values.
    M = atoi(line1);
    n = atoi(line2);

    int A[n];
    A[0] = atoi(strtok(line3, space));
    for (int i = 1; i < n; i++)
    {
        A[i] = atoi(strtok(NULL, space));
    }

    // Close the file.
    fclose(input_file);

    // Create 2 child processes.
    int f;
    for (i = 0; i < 2; i++)
    {
        f = fork();
        if (f < 0)
        {
            printf("FORK error!\n");
            exit(1);
        }
        if (f == 0)
        {
            // If child process, break the loop and continue.
            break;
        }
        // If parent process. Keep the child ID and loop again for second child.
        children[i] = f;
    }

    // Parent process code goes here.
    if (f != 0)
    {
        // Creating a semaphore between parent and child 1.
        parent_sem = semget(KEYSEM1, 1, 0700|IPC_CREAT);
        semctl(parent_sem, 0, SETVAL, 0); // 0 for semaphore.

        // Creating a semaphore between child 0 and child 1.
        child_sem = semget(KEYSEM2, 1, 0700|IPC_CREAT);
        semctl(child_sem, 0, SETVAL, 0); // 0 for semaphore.

        // Creating a shared memory area with enough size.
        shm_id = shmget(KEYSHM, (2 * n + 4) * sizeof(int), 0700|IPC_CREAT);
        
        // Attaching the shared memory segment identified by shm_id.
        // to the address space of the calling process. (parent)
        globalcp = (int*)shmat(shm_id, 0, 0);

        // Initial values.
        *globalcp       = n;
        *(globalcp + 1) = M;
        *(globalcp + 2) = 0; // x
        *(globalcp + 3) = 0; // y
        for (int a = 0; a < n; a++) { *(globalcp + 4 + a) = A[a]; }

        // Detaching the shared memory segment from the address
        // to the space of the calling process. (parent)
        shmdt(globalcp);
        
        // Wait 2 seconds for children to be ready.
        sleep(2);

        // Wake up the children.
        for (int i = 0; i < 2; i++) { kill(children[i], 12); }

        // Wait 2 seconds again for children to be ready.
        sleep(2);

        // Wait signal from child1 to finish.
        semWait(parent_sem, 1);

        // Attach the shared memory.
        globalcp = (int*)shmat(shm_id, 0, 0);

        // Write the values from shared memory to the output file.
        FILE* out_file;
        out_file = fopen(argv[2], "w");

        fprintf(out_file, "%d\n", *(globalcp + 1));                             // M
        fprintf(out_file, "%d\n", *globalcp);                                   // n
        for (int i = 0; i < n; i ++)
        {
            fprintf(out_file,"%d ", *(globalcp + 4 + i));                       // A array
        }
        fprintf(out_file, "\n%d\n", *(globalcp + 2));                           // x
        for (int i = 0; i < *(globalcp + 2); i ++)
        {
            fprintf(out_file,"%d ", *(globalcp + 4 + n + i));                   // B array
        }
        fprintf(out_file, "\n%d\n", *(globalcp + 3));                           // y
        for (int i = 0; i < *(globalcp + 3); i ++)
        {
            fprintf(out_file,"%d ", *(globalcp + 4 + n + *(globalcp + 2) + i)); // C array
        }

        // Close the file.
        fclose(out_file);

        // Detach the shared memory.
        shmdt(globalcp);

        // Remove the created semaphores and shared memory.
        semctl(parent_sem, 0, IPC_RMID, 0);
        semctl(child_sem, 0, IPC_RMID, 0);
        shmctl(shm_id, IPC_RMID, 0);
        
        // Parent process is exiting.
        exit(0);
    }

    else // Children processes code goes here.
    {
        // Wait until receiving a signal.
        pause();
        my_order = i; // Child number.

        // Set the semaphores and shared memory.
        parent_sem = semget(KEYSEM1, 1, 0);
        child_sem  = semget(KEYSEM2, 1, 0);
        shm_id     = shmget(KEYSHM, (2 * n + 4) * sizeof(int), 0);
        
        // Attach the shared memory.
        globalcp = (int*)shmat(shm_id, 0, 0);

        if (my_order == 0) // Child 0
        {
            printf("child %d is started writing\n", my_order);

            // Write B array.
            for (int i = 0; i < n; i++)
            {
                if (A[i] <= M)
                {
                    *(globalcp + n + 4 + counter) = A[i];
                    counter++;
                }
            }

            // Write x and y values.
            *(globalcp + 2) = counter;     // x
            *(globalcp + 3) = n - counter; // y = n - x

            // Detach the shared memory.
            shmdt(globalcp);

            // After writing, send signal to the child1.
            semSignal(child_sem, 1);
        }
        else if (my_order == 1) // Child 1
        {
            // Wait for child0 to finish writing.
            semWait(child_sem, 1);

            printf("child %d is started writing\n", my_order);

            // Get counter from where it left.
            counter = *(globalcp + 2);
            for (int i = 0; i < n; i++)
            {
                // Write C array.
                if (A[i] > M)
                {
                    *(globalcp + n + 4 + counter) = A[i];
                    counter++;
                }
            }

            // Detach the shared memory.
            shmdt(globalcp);

            // After writing, send signal to the parent.
            semSignal(parent_sem, 1);
        }

        // Child process is exiting.
        exit(0);
    }
    return 0;
}
