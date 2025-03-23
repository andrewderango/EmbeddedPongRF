#include "mbed.h"
#include "nRF24L01P.h"
#include <cstdio>
#define TRANSFER_SIZE 32

// mosi, miso, sck, nsc, ce, irq
// no PC1, PC0, PF10, PF9, PF5, PF3, PF1, PC15, PF6, PA3, BTN1
nRF24L01P transmitter(PE_14, PE_13, PE_12, PE_11, PE_9);

void print_diagnostic_info();

int main() {
    char rxData[TRANSFER_SIZE];
    int rxDataCount;
    transmitter.powerUp();
    printf("[RX Board]\n");
    print_diagnostic_info();
    transmitter.setTransferSize(TRANSFER_SIZE);
    transmitter.setReceiveMode();
    transmitter.enable();

    while (true) {
        if (transmitter.readable()) {
            rxDataCount = transmitter.read(NRF24L01P_PIPE_P0, rxData, sizeof(rxData));
            printf("[RX Board] Received: %s\n", rxData);
        }
    }
}

void print_diagnostic_info() {
    printf("[RX Board] Frequency    : %d MHz\n", transmitter.getRfFrequency());
    printf("[RX Board] Output power : %d dBm\n", transmitter.getRfOutputPower());
    printf("[RX Board] Data rate    : %d kbps\n", transmitter.getAirDataRate());
    printf("[RX Board] Frequency    : %d MHz\n", transmitter.getRfFrequency());
    printf("[RX Board] Output power : %d dBm\n", transmitter.getRfOutputPower());
    printf("[RX Board] Data rate    : %d kbps\n", transmitter.getAirDataRate());
    printf("[RX Board] TX Address   : 0x%llx\n", transmitter.getTxAddress());
    printf("[RX Board] RX Address   : 0x%llx\n", transmitter.getRxAddress());
}