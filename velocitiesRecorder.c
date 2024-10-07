#include <khepera/khepera.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// Constants
#define KH4_PULSE_TO_MM (1000.0 / 370.0)  // Conversion factor for pulse to mm
#define SAMPLING_INTERVAL 50000           // Sampling interval in microseconds (50ms)

// Global Variables
static knet_dev_t *dsPic;   // Khepera IV device handle
FILE *velocityFile;         // File to save velocities
int leftPosPrev = 0, rightPosPrev = 0;    // Previous encoder positions

// Function Prototypes
void initializeKhepera();
void finalizeKhepera();
void recordVelocities();
void shutdownHandler(int sig);
long long getTimeDiff(struct timeval *start, struct timeval *end);

int main() {
    // Initialize Khepera IV
    initializeKhepera();

    // Set up signal handler for safe shutdown
    signal(SIGINT, shutdownHandler);

    // Open file to save velocities
    velocityFile = fopen("robot_velocities_log.txt", "w");
    if (!velocityFile) {
        perror("Error opening file for writing velocities");
        finalizeKhepera();
        exit(EXIT_FAILURE);
    }

    // Main loop to record velocities
    while (1) {
        recordVelocities();
        usleep(SAMPLING_INTERVAL);  // Wait for the next sampling interval
    }

    // Finalize Khepera IV (This line will only be reached on error)
    finalizeKhepera();
    return 0;
}

void initializeKhepera() {
    // Initialize libkhepera
    if (kh4_init(0, NULL) != 0) {
        fprintf(stderr, "Failed to initialize Khepera IV library\n");
        exit(EXIT_FAILURE);
    }

    // Open connection to Khepera IV
    dsPic = knet_open("Khepera4:dsPic", KNET_BUS_I2C, 0, NULL);
    if (!dsPic) {
        fprintf(stderr, "Failed to open Khepera IV device\n");
        exit(EXIT_FAILURE);
    }

    // Reset encoders
    kh4_ResetEncoders(dsPic);
    kh4_get_position(&leftPosPrev, &rightPosPrev, dsPic);  // Initial positions

    printf("Khepera IV initialized successfully\n");
}

void finalizeKhepera() {
    // Close file if open
    if (velocityFile) {
        fclose(velocityFile);
    }

    // Close connection
    knet_close(dsPic);

    printf("Khepera IV shutdown safely\n");
}

// Function to record the velocities to the file
void recordVelocities() {
    int leftPos, rightPos;
    double leftVel, rightVel;
    struct timeval currentTime;
    static struct timeval lastTime;

    // Get current encoder positions
    kh4_get_position(&leftPos, &rightPos, dsPic);

    // Calculate the time difference since the last recording
    gettimeofday(&currentTime, NULL);
    long long timeDiffUs = getTimeDiff(&lastTime, &currentTime);  // Time in microseconds

    if (timeDiffUs > 0) {
        // Calculate velocities in mm/s
        leftVel = ((leftPos - leftPosPrev) * KH4_PULSE_TO_MM * 1000000.0) / timeDiffUs;
        rightVel = ((rightPos - rightPosPrev) * KH4_PULSE_TO_MM * 1000000.0) / timeDiffUs;

        // Get current time as a string
        char timestamp[30];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(timestamp, 30, "%Y-%m-%d %H:%M:%S", tm_info);

        // Log the velocities with timestamp
        fprintf(velocityFile, "[%s] Left Velocity: %.2f mm/s, Right Velocity: %.2f mm/s\n",
                timestamp, leftVel, rightVel);
        fflush(velocityFile);  // Ensure data is written immediately

        // Update the previous positions and time
        leftPosPrev = leftPos;
        rightPosPrev = rightPos;
        lastTime = currentTime;
    }
}

// Function to get time difference in microseconds
long long getTimeDiff(struct timeval *start, struct timeval *end) {
    return (1000000LL * (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec));
}

void shutdownHandler(int sig) {
    printf("Program interrupted. Shutting down safely.\n");
    finalizeKhepera();
    exit(EXIT_SUCCESS);
}


