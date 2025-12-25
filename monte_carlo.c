/**
 * monte_carlo.c
 * Parallel Monte Carlo Pi Estimation.
 * Uses fork() and shared memory (mmap) to aggregate results.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_processes> <total_points>\n", argv[0]);
        return 1;
    }

    int num_processes = atoi(argv[1]);
    long total_points = atol(argv[2]);
    long points_per_process = total_points / num_processes;

    if (num_processes <= 0 || total_points <= 0) {
        printf("Arguments must be positive integers.\n");
        return 1;
    }

    // Allocate shared memory for results (array of longs, one per process)
    long *results = mmap(NULL, num_processes * sizeof(long),
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (results == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    printf("Estimating Pi using %d processes and %ld total points...\n", 
           num_processes, total_points);

    pid_t pid;
    for (int i = 0; i < num_processes; i++) {
        pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (pid == 0) { // Child Process
            // Unique seed for each process
            unsigned int seed = time(NULL) ^ (getpid() << 16);
            long points_in_circle = 0;
            double x, y;

            for (long j = 0; j < points_per_process; j++) {
                // Generate random double between -1.0 and 1.0
                // rand_r is thread/process safe given unique seed
                x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
                y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;

                if (x * x + y * y <= 1.0) {
                    points_in_circle++;
                }
            }

            // Write result to shared memory slot
            results[i] = points_in_circle;
            exit(0);
        }
    }

    // Parent waits for all children
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }

    // Aggregate results
    long total_in_circle = 0;
    for (int i = 0; i < num_processes; i++) {
        total_in_circle += results[i];
    }

    // Calculate Pi
    double pi_estimate = 4.0 * (double)total_in_circle / (double)total_points;
    printf("Pi Estimate: %f\n", pi_estimate);

    // Clean up
    munmap(results, num_processes * sizeof(long));

    return 0;
}