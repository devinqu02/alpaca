#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NV_MEMORY_SIZE 65536  // 64 KB

/**
 * Simulates power loss by terminating and restarting the provided program.
 * If the application exits normally, stop the simulation and finish.
 *
 * @param program Path to the application binary to be executed.
 */
void simulate_power_loss(const char *program) {
    pid_t child_pid = fork();
    if (child_pid == 0) {
        // Child process: Launch the application
        setenv("FIRST_RUN", "1", 1);  // Mark this as the first run
        execlp(program, program, NULL);
        perror("execlp");  // Report error if execlp fails
        exit(1);
    } else if (child_pid > 0) {
        // Parent process:
        unsetenv("FIRST_RUN");  // Reset for subsequent runs

        while (1) {
            // Wait for the child process to finish
            int status;
            pid_t result = waitpid(child_pid, &status, 0);
            if (result == -1) {
                perror("waitpid");
                exit(1);
            }

            // Restart the application
            child_pid = fork();
            if (child_pid == 0) {
                // Child process: Relaunch the application
                execlp(program, program, NULL);
                perror("execlp");
                exit(1);
            } else if (child_pid < 0) {
                perror("fork");
                exit(1);
            }
        }
    } else {
        perror("fork");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <application_binary>\n", argv[0]);
        return 1;
    }

    const char *shm_name = "sram_emulated";  // Shared memory name

    // Create shared memory for simulated non-volatile memory
    int shm_sram_emulated = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_sram_emulated == -1) {
        perror("shm_open");
        return 1;
    }

    // Resize shared memory to the desired size
    if (ftruncate(shm_sram_emulated, NV_MEMORY_SIZE) == -1) {
        perror("ftruncate");
        shm_unlink(shm_name);  // Clean up shared memory
        return 1;
    }

    // Map the shared memory into the process's address space
    void *nv_memory = mmap(NULL, NV_MEMORY_SIZE, PROT_READ | PROT_WRITE,
                           MAP_SHARED, shm_sram_emulated, 0);
    if (nv_memory == MAP_FAILED) {
        perror("mmap");
        shm_unlink(shm_name);
        return 1;
    }

    setenv("NV_MEMORY_SHM_NAME", shm_name, 1);  // Pass shm name

    printf("Simulated NV memory allocated at %p\n", nv_memory);

    // Simulate power cycles with the given application binary
    simulate_power_loss(argv[1]);

    // Clean up shared memory
    munmap(nv_memory, NV_MEMORY_SIZE);
    shm_unlink(shm_name);
    return 0;
}
