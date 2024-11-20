#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define NV_MEMORY_SIZE 65536  // Size of the simulated non-volatile memory
#define POWER_ON_TIME_MS 100

void *nv_memory;
extern char __start_nv_data[];
extern char __stop_nv_data[];

// Initialize shared memory and map it to virtual memory
void initialize_memory() {
    const char *shm_name = getenv("NV_MEMORY_SHM_NAME");
    if (!shm_name) {
        fprintf(stderr,
                "Error: NV_MEMORY_SHM_NAME environment variable is not set.\n");
        exit(1);
    }

    int shm_sram_emulated = shm_open(shm_name, O_RDWR, 0666);
    if (shm_sram_emulated == -1) {
        perror("shm_open");
        exit(1);
    }

    nv_memory = mmap(NULL, NV_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                     shm_sram_emulated, 0);
    if (nv_memory == MAP_FAILED) {
        perror("mmap");
        close(shm_sram_emulated);
        exit(1);
    }

    if (!getenv("FIRST_RUN")) {
        memcpy(__start_nv_data, nv_memory, (__stop_nv_data - __start_nv_data));
    }
}

// Save memory state to the shared memory
void save_memory(void) {
    memcpy(nv_memory, __start_nv_data, (__stop_nv_data - __start_nv_data));
    fprintf(stderr, "Non-volatile data saved to memory.\n");
}

// Signal handler for power-off simulation
void on_power_off(int sig) {
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == -1) {
        perror("clock_gettime");
        exit(1);
    }

    printf("Power off simulated: CPU time run = %ld.%09ld seconds\n", ts.tv_sec,
           ts.tv_nsec);
    save_memory();
    exit(0);
}

// Set up a timer for power-off simulation
void set_up_timer(void) {
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &timerid;

    if (timer_create(CLOCK_PROCESS_CPUTIME_ID, &sev, &timerid) == -1) {
        perror("timer_create");
        exit(1);
    }

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = POWER_ON_TIME_MS * 1000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = on_power_off;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

// Function to run before main
__attribute__((constructor)) void emulator_setup() {
    initialize_memory();
    set_up_timer();
}
