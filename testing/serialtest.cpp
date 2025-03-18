#include "mbed.h"

// Define master SPI pins
#define MASTER_MOSI PE_6 // white
#define MASTER_MISO PE_5 // grey
#define MASTER_SCLK PE_2 // yellow
#define MASTER_SSEL PE_4 // blue

// Define slave SPI pins
#define SLAVE_MOSI PE_14 // white
#define SLAVE_MISO PE_13 // grey
#define SLAVE_SCLK PE_12 // yellow
#define SLAVE_SSEL PE_11 // blue

// Master and Slave SPI instances
SPI master(MASTER_MOSI, MASTER_MISO, MASTER_SCLK);
SPISlave slave(SLAVE_MOSI, SLAVE_MISO, SLAVE_SCLK, SLAVE_SSEL);
DigitalOut chip_select(MASTER_SSEL);

// Main function
int main() {

    printf("=============\n");

    // Initial response from slave
    slave.reply(99);
    bool donc = true;
    int timeout_counter = 0;
    const int timeout_limit = 100; // Timeout limit to avoid infinite loop

    while (donc) {
        // Step 1: Master initiates communication
        printf("Master: Initiating communication\n");
        master.lock();
        chip_select = 0;
        int master_response = master.write(0x01); // Write 1 to slave
        chip_select = 1;
        master.unlock();
        printf("Master: Data sent, response: %d\n", master_response);

        // Small delay to ensure synchronization
        ThisThread::sleep_for(10ms);

        // Step 2: Slave processes received data
        if (slave.receive()) {
            int received_from_master = slave.read(); // Read from master
            printf("Slave: Received %d from master\n", received_from_master);
            slave.reply(received_from_master + 1);   // Reply with received + 1
            donc = false; // End the loop after successful communication
        } else {
            printf("Slave: No data received\n");
        }

        // Small delay to avoid busy-waiting
        ThisThread::sleep_for(10ms);

        // Increment timeout counter
        timeout_counter++;
        if (timeout_counter >= timeout_limit) {
            printf("Timeout: No data received from master\n");
            donc = false;
        }
    }

    printf("Communication ended\n");
}