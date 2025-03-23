#include "mbed.h"
#include "nRF24L01P.h"
#include <vector>

#define TRANSFER_SIZE 30

// mosi, miso, sck, nsc, ce, irq
nRF24L01P transmitter(PE_14, PE_13, PE_12, PE_11, PE_9);

void print_diagnostic_info();
void transmit_game_state(uint8_t num_balls, const std::vector<std::pair<int, int>>& ball_positions, int paddle1_pos, int paddle2_pos, int score1, int score2);

int main() {
    printf("\n\n===============\n\n");

    transmitter.powerUp();
    printf("[TX Board]\n");
    print_diagnostic_info();
    transmitter.setTransferSize(TRANSFER_SIZE);
    transmitter.setTransmitMode();
    transmitter.enable();

    while (true) {
        uint8_t num_balls = 2;
        std::vector<std::pair<int, int>> ball_positions = {{10, 20}, {30, 40}};
        int paddle1_pos = 50;
        int paddle2_pos = 60;
        int score1 = 5;
        int score2 = 3;

        transmit_game_state(num_balls, ball_positions, paddle1_pos, paddle2_pos, score1, score2);
        ThisThread::sleep_for(500ms);
    }
}

void transmit_game_state(uint8_t num_balls, const std::vector<std::pair<int, int>>& ball_positions, int paddle1_pos, int paddle2_pos, int score1, int score2) {
    char message[TRANSFER_SIZE] = {0};
    message[0] = num_balls;

    for (size_t i = 0; i < ball_positions.size() && i < 8; ++i) {
        int x = ball_positions[i].first;
        int y = ball_positions[i].second;
        message[1 + i * 3] = x & 0xFF;
        message[2 + i * 3] = y & 0xFF;
        message[3 + i * 3] = (y >> 8) & 0xFF;
    }

    message[25] = paddle1_pos & 0xFF;
    message[26] = paddle2_pos & 0xFF;

    message[27] = score1 & 0xFF;
    message[28] = score2 & 0xFF;

    message[29] = 2; // Game state: 0 = Menu, 1 = Pause, 2 = Game

    int bits_written = transmitter.write(NRF24L01P_PIPE_P0, message, TRANSFER_SIZE);
    printf("bits written: %d\n", bits_written);
    printf("[TX Board] Transmitting \"");
    for (int i = 0; i < TRANSFER_SIZE; ++i) {
        printf("%02X ", message[i]);
    }
    printf("\"\n");
}

void print_diagnostic_info() {
    printf("[TX Board] Frequency    : %d MHz\n", transmitter.getRfFrequency());
    printf("[TX Board] Output power : %d dBm\n", transmitter.getRfOutputPower());
    printf("[TX Board] Data rate    : %d kbps\n", transmitter.getAirDataRate());
    printf("[TX Board] TX Address   : 0x%llx\n", transmitter.getTxAddress());
    printf("[TX Board] RX Address   : 0x%llx\n", transmitter.getRxAddress());
}