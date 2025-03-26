#include "mbed.h"
#include "nRF24L01P.h"

#define TRANSFER_SIZE 32

// transmitter: DISCO-F429ZI: 066DFF4951775177514867255038 (AV2)
// receiver: DISCO-F429ZI: 066CFF545150898367163727 (AV1)

// mosi, miso, sck, nsc, ce, irq
// no PC1, PC0, PF10, PF9, PF5, PF3, PF1, PC15, PF6, PA3, BTN1
nRF24L01P transmitter(PE_14, PE_13, PE_12, PE_11, PE_9, NC);

void print_diagnostic_info();

int main() {

    printf("\n\n===============\n\n");

    transmitter.powerUp();
    printf("[TX Board]\n");
    print_diagnostic_info();
    transmitter.setTransferSize(TRANSFER_SIZE);
    transmitter.setReceiveMode();
    transmitter.enable();

    while (true) {
        char* msg = "ABABABABABABABABABABABABABABABA\0";
        int bits_written = transmitter.write(NRF24L01P_PIPE_P0, msg, TRANSFER_SIZE);
        printf("bits written: %d\n", bits_written);
        printf("[TX Board] Transmitting \"%s\"\n", msg);
        ThisThread::sleep_for(500ms);
    }
}

void print_diagnostic_info() {
    printf("[TX Board] Frequency    : %d MHz\n", transmitter.getRfFrequency());
    printf("[TX Board] Output power : %d dBm\n", transmitter.getRfOutputPower());
    printf("[TX Board] Data rate    : %d kbps\n", transmitter.getAirDataRate());
    printf("[TX Board] TX Address   : 0x%llx\n", transmitter.getTxAddress());
    printf("[TX Board] RX Address   : 0x%llx\n", transmitter.getRxAddress());
}