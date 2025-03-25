#include "mbed.h"
#include "nRF24L01P.h"
#define TRANSFER_SIZE 30

// transmitter: DISCO-F429ZI: 066CFF545150898367163727 (AV1)
// receiver: DISCO-F429ZI: 066DFF4951775177514867255038 (AV2)

// mosi, miso, sck, nsc, ce, irq
nRF24L01P receiver(PE_14, PE_13, PE_12, PE_11, PE_9, NC);

void print_diagnostic_info();

int main() {
    char rxData[TRANSFER_SIZE];
    int rxDataCount;
    receiver.powerUp();
    printf("[RX Board]\n");
    print_diagnostic_info();
    receiver.setTransferSize(TRANSFER_SIZE);
    receiver.setReceiveMode();
    receiver.enable();

    while (true) {
        if (receiver.readable()) {
            rxDataCount = receiver.read(NRF24L01P_PIPE_P0, rxData, sizeof(rxData));
            // printf("Data Count: %d\n", rxDataCount);
            // interpret the rxData
            uint8_t num_balls = rxData[0];
            printf("Number of balls: %d\n", num_balls);
            for (size_t i = 0; i < num_balls; ++i) {
                int x = (rxData[1 + i * 3] & 0xFF) | ((rxData[2 + i * 3] & 0xFF) << 8);
                int y = (rxData[3 + i * 3] & 0xFF);
                printf("Ball %d: (%d, %d)\n", i, x, y);
            }
            int paddle1_pos = rxData[25];
            int paddle2_pos = rxData[26];
            printf("Paddle 1: %d\n", paddle1_pos);
            printf("Paddle 2: %d\n", paddle2_pos);
            int score1 = rxData[27];
            int score2 = rxData[28];
            printf("Score 1: %d\n", score1);
            printf("Score 2: %d\n", score2);
            int game_state = rxData[29];
            printf("Game state: %d\n", game_state);
        }
    }
}

void print_diagnostic_info() {
    printf("[RX Board] Frequency    : %d MHz\n", receiver.getRfFrequency());
    printf("[RX Board] Output power : %d dBm\n", receiver.getRfOutputPower());
    printf("[RX Board] Data rate    : %d kbps\n", receiver.getAirDataRate());
    printf("[RX Board] TX Address   : 0x%08X\n", receiver.getTxAddress());
    printf("[RX Board] RX Address   : 0x%08X\n", receiver.getRxAddress());
}