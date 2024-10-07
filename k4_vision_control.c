
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <khepera/khepera.h>
#include <signal.h>

#define IP "192.168.1.3"
#define PORT 5544
#define SIZE 1024

int client_fd;
static knet_dev_t *dsPic;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("Received interrupt signal. Cleaning up...\n");
        kh4_SetMode(kh4RegSpeed, dsPic);
        kh4_set_speed(0, 0, dsPic);
        usleep(4000000);
        // Close socket and perform other clean-up tasks
        close(client_fd);

        exit(EXIT_SUCCESS); // Exit the program gracefully
    }
}

void turnLeft90Degrees() {
    int duration = 4000000;
    int speedLeft = -20;
    int speedRight = 40;
    kh4_SetMode(kh4RegSpeed, dsPic);
    kh4_set_speed(speedLeft, speedRight, dsPic);
    usleep(duration);
    kh4_set_speed(0, 0, dsPic);
}

void turnRight90Degrees() {
    int duration = 4000000;
    int speedLeft = 40;
    int speedRight = -20;
    kh4_SetMode(kh4RegSpeed, dsPic);
    kh4_set_speed(speedLeft, speedRight, dsPic);
    usleep(duration);
    kh4_set_speed(0, 0, dsPic);
}

// Add other functions (turnRight90Degrees, stop) if needed

int main() {
    signal(SIGINT, signal_handler);
    // robot initialization
    if (kh4_init(0, NULL) != 0) {
        printf("\nERROR: could not initiate the libkhepera!\n\n");
        return -1;
    }

    dsPic = knet_open("Khepera4:dsPic", KNET_BUS_I2C, 0, NULL);

    if (dsPic == NULL) {
        printf("\nERROR: could not initiate communication with Kh4 dsPic\n\n");
        return -2;
    }

    // Start moving forward initially
    int speedLeft = 30;
    int speedRight = 30;
    kh4_SetMode(kh4RegSpeed, dsPic);
    kh4_set_speed(speedLeft, speedRight, dsPic);

    struct sockaddr_in server_addr;

    // Create a socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Configure the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        printf("STOPPING THE ROBOT\n");
        kh4_SetMode(kh4RegSpeed, dsPic);
        kh4_set_speed(0, 0, dsPic);

        return 1;
    }

    // Loop to sense objects and take actions
    while (1) {
        // Receive a value from the server
        char prediction[SIZE];
        ssize_t bytes_received = recv(client_fd, prediction, SIZE - 1, 0);

        if (bytes_received < 0) {
            perror("Failed to receive the value from the server");
            close(client_fd);
            return -11;
        }

        // Null-terminate the received data to treat it as a string
        prediction[bytes_received] = '\0';

        printf("Received prediction: %s\n", prediction);

        // Execute the function based on the received value
        if (strcmp(prediction, "right") == 0) {
            turnRight90Degrees();
            // Start moving forward again
            kh4_SetMode(kh4RegSpeed, dsPic);
            kh4_set_speed(30, 30, dsPic);
        }
        else if (strcmp(prediction,"left") == 0){
        	turnLeft90Degrees();
        	// Start moving forward again
        	kh4_SetMode(kh4RegSpeed, dsPic);
        	kh4_set_speed(30, 30, dsPic);
        	}
        else if (strcmp(prediction,"stop") == 0){
        	kh4_SetMode(kh4RegSpeed, dsPic);
        	kh4_set_speed(0, 0, dsPic);
        }
        else if (strcmp(prediction,"thirty") == 0){
            kh4_SetMode(kh4RegSpeed, dsPic);
            kh4_set_speed(60, 60, dsPic);
        }
        else if (strcmp(prediction,"sixty") == 0){
            kh4_SetMode(kh4RegSpeed, dsPic);
            kh4_set_speed(120, 120, dsPic);
        }

        // Exit the loop after processing
    }

    // Sleep for a while to avoid high CPU usage
    usleep(100000); // Sleep for 100 milliseconds

    return 0;
}

