#include "functions.h"
#include "LCD_DISCO_F429ZI.h"
#include "DebouncedInterrupt.h"
#include "nRF24L01P.h"
#include "mbed.h"
#include <time.h>
#include <type_traits>
#include <vector>
#include <cstdlib>

#define RCC_AHB2ENR    (*(volatile uint32_t *)(RCC_BASE + 0x34))    // AHB2 Peripheral Clock Enable register
#define RNG_CR         (*(volatile uint32_t *)(RNG_BASE + 0x00))    // RNG Control register
#define RNG_SR         (*(volatile uint32_t *)(RNG_BASE + 0x04))    // RNG Status register
#define RNG_DR         (*(volatile uint32_t *)(RNG_BASE + 0x08))    // RNG Data register

#define MASTER 1 // 1 for master, 0 for slave
#define MASTER_TRANSFER_SIZE 32 // 30 byte RF payload
#define SLAVE_TRANSFER_SIZE 1 // 1 byte RF payload
#define TICKERTIME 20ms
#define AI1_DIFFICULTY 1 // 0 is easy, 10 is hard (top paddle)
#define AI2_DIFFICULTY 3 // 0 is easy, 10 is hard (bottom paddle)

// master: DISCO-F429ZI: 066CFF545150898367163727 (AV1)
// slave: DISCO-F429ZI: 066DFF4951775177514867255038 (AV2)

// DEVICES --------------------------------

LCD_DISCO_F429ZI LCD;
nRF24L01P master(PE_14, PE_13, PE_12, PE_11, PE_9, NC); // MOSI, MISO, SCK, CS, CE, IRQ
nRF24L01P slave(PE_14, PE_13, PE_12, PE_11, PE_9, NC); // MOSI, MISO, SCK, CS, CE, IRQ
DigitalOut red_led(PG_13);
DigitalOut green_led(PG_14);

// INTERRUPTS -----------------------------

InterruptIn onboard_button(BUTTON1);
DebouncedInterrupt external_button1(PA_5);
DebouncedInterrupt external_button2(PA_6);
DebouncedInterrupt external_button3(PA_7);
DebouncedInterrupt external_button4(PG_3);
DebouncedInterrupt external_button5(PH_1);
DebouncedInterrupt external_button6(PG_2);
Ticker game_ticker;

// DATA TYPES -----------------------------

typedef enum {
    STATE_MENU = 0,
    STATE_PAUSE = 1,
    STATE_GAME = 2,
} StateType;

// GLOBAL VARS ----------------------------

static StateType curr_state;
static StateType prev_state = STATE_GAME;
bool spawn_ball_flag = false;

// OBJECTS --------------------------------
// BOARD OBJECT METHODS
        
// Constructor
Board::Board(int min_width, int min_height, int max_width, int max_height) : min_width(min_width), min_height(min_height), max_width(max_width), max_height(max_height) {
    rngInit();
    balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);
    paddles.emplace_back((int)((float)(max_width-min_width) / 2 - (0.15 * (max_width-min_width)) / 2), min_height + 5, *this);
    paddles.emplace_back((int)((float)(max_width-min_width) / 2 - (0.15 * (max_width-min_width)) / 2), max_height - 10, *this);
    score1 = 0;
    score2 = 0;
}

// Destructor
Board::~Board() {}

int Board::getMinHeight() const { return min_height; }
int Board::getMinWidth() const { return min_width; }
int Board::getMaxHeight() const { return max_height; }
int Board::getMaxWidth() const { return max_width; }
void Board::drawBalls() {
    for (int i = 0; i < balls.size(); i++) {
        balls[i].draw();
    }
}
void Board::moveBalls() {
    for (int i = 0; i < balls.size(); i++) {
        bool delete_ball = false;
        balls[i].move(*this, delete_ball);

        // somehow changing the LCD in an ISR???
        if (delete_ball) {
            LCD.SetTextColor(LCD_COLOR_BLACK);
            LCD.FillCircle(balls[i].getLastDrawnX(), balls[i].getLastDrawnY(), 3);
            // LCD.FillCircle(balls[i].getx(), balls[i].gety(), 3);
            balls.erase(balls.begin() + i);
            i--;
        }
    }
    if (balls.size() <= 0) {
        balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);
    }
    
    // AI opponent
    float rand_num = randBetween(0,11);
    if (balls[0].getx() < paddles[0].getLeft() && ai1_enabled && rand_num < AI1_DIFFICULTY) {
        paddles[0].moveLeft();
    } else if (balls[0].getx() > paddles[0].getRight() && ai1_enabled && rand_num < AI1_DIFFICULTY) {
        paddles[0].moveRight();
    }
    if (balls[0].getx() < paddles[1].getLeft() && ai2_enabled && rand_num > 11-AI2_DIFFICULTY) {
        paddles[1].moveLeft();
    } else if (balls[0].getx() > paddles[1].getRight() && ai2_enabled && rand_num > 11-AI2_DIFFICULTY) {
        paddles[1].moveRight();
    }
}
void Board::spawnBall() {
    if (balls.size() < maxNumOfBalls) {
        balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);
    }
}
int Board::transmitBoardState(bool verbose) {
    // pull data from board object
    master.setTransferSize(SLAVE_TRANSFER_SIZE);
    char message[MASTER_TRANSFER_SIZE] = {0};
    if (curr_state == STATE_GAME) {
        uint8_t num_balls = balls.size();
        std::vector<std::pair<int, int>> ball_positions;
        for (int i = 0; i < num_balls; i++) {
            ball_positions.push_back(std::make_pair(balls[i].getx(), balls[i].gety()));
        }
        int paddle1_pos = paddles[0].getLeft();
        int paddle2_pos = paddles[1].getLeft();

        // format the data under defined protocol
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
        message[27] = this->score1 & 0xFF;
        message[28] = (this->score1 >> 8) & 0xFF;
        message[29] = this->score2 & 0xFF;
        message[30] = (this->score2 >> 8) & 0xFF;
    }
    message[31] = curr_state;

    // transmit the data
    int bits_written = master.write(NRF24L01P_PIPE_P0, message, MASTER_TRANSFER_SIZE);

    if (verbose) {
        printf("[Master] %d || ", bits_written);
        for (int i = 0; i < MASTER_TRANSFER_SIZE; ++i) {
            printf("%02X ", message[i]);
        }
        printf("\n");
    }

    return bits_written;
}
int Board::processIncomingSlaveMessage(bool verbose) {
    if (master.readable()) {
        master.setTransferSize(SLAVE_TRANSFER_SIZE);
        char slave_message[SLAVE_TRANSFER_SIZE] = {0};
        int bits_read = master.read(NRF24L01P_PIPE_P0, slave_message, 1);
        if (bits_read > 0) {
            int slave_paddle_pos = slave_message[0] & 0xFF;
            paddles[1].moveTo(slave_paddle_pos);
        }
        if (verbose) {
            printf("[Slave] %d || ", bits_read);
            for (int i = 0; i < 1; ++i) {
                printf("%02X ", slave_message[i]);
            }
            printf("\n");
        }
        return bits_read;
    }

    return 0;
}
int Board::processIncomingMasterMessage(bool verbose) {
    if (slave.readable()) {
        slave.setTransferSize(MASTER_TRANSFER_SIZE);
        char master_message[MASTER_TRANSFER_SIZE] = {0};
        int bits_read = master.read(NRF24L01P_PIPE_P0, master_message, MASTER_TRANSFER_SIZE);

        if (verbose) {
            printf("[Master] %d || ", bits_read);
            for (int i = 0; i < MASTER_TRANSFER_SIZE; ++i) {
                printf("%02X ", master_message[i]);
            }
            printf("\n");
        }

        if (bits_read > 0) {
            if (master_message[31] == 2) {
                curr_state = STATE_GAME;

                // parse the received data
                uint8_t num_balls = master_message[0];
                std::vector<std::pair<int, int>> ball_positions;
                for (int i = 0; i < num_balls; i++) {
                    int x = (master_message[1 + i * 3] & 0xFF) | ((master_message[2 + i * 3] & 0xFF) << 8);
                    int y = (master_message[3 + i * 3] & 0xFF);
                    ball_positions.push_back(std::make_pair(x, y));
                }
                int paddle1_pos = master_message[25];
                int paddle2_pos = master_message[26];
                int score1 = (master_message[27] & 0xFF) | ((master_message[28] & 0xFF) << 8);
                int score2 = (master_message[29] & 0xFF) | ((master_message[30] & 0xFF) << 8);
                
                // update the board object with the received data
                balls.clear();
                for (int i = 0; i < num_balls; i++) {
                    balls.emplace_back(ball_positions[i].first, ball_positions[i].second);
                }
                paddles[0].moveTo(paddle1_pos);
                paddles[1].moveTo(paddle2_pos);
                this->score1 = score1;
                this->score2 = score2;

                // update the balls on the screen
                for (int i = 0; i < balls.size(); i++) {
                    bool delete_ball = false;
                    balls[i].move(*this, delete_ball);
            
                    if (delete_ball) {
                        LCD.SetTextColor(LCD_COLOR_BLACK);
                        LCD.FillCircle(balls[i].getLastDrawnX(), balls[i].getLastDrawnY(), 3);
                        // LCD.FillCircle(balls[i].getx(), balls[i].gety(), 3);
                        balls.erase(balls.begin() + i);
                        i--;
                    }
                }

            } else if (master_message[31] == 1) {
                curr_state = STATE_PAUSE;
            } else if (master_message[31] == 0) {
                curr_state = STATE_MENU;
            }
        }

        return bits_read;
    }
    return 0;
}
int Board::transmitOutboundSlaveMessage(bool verbose) {
    slave.setTransferSize(SLAVE_TRANSFER_SIZE);
    char message[1] = {0};
    message[0] = paddles[1].getLeft() & 0xFF;
    int bits_written = slave.write(NRF24L01P_PIPE_P0, message, SLAVE_TRANSFER_SIZE);

    if (verbose) {
        printf("[Slave] %d || ", bits_written);
        for (int i = 0; i < SLAVE_TRANSFER_SIZE; ++i) {
            printf("%02X ", message[i]);
        }
        printf("\n");
    }

    return bits_written;
}
void Board::incrementScore1() { score1++; }
void Board::incrementScore2() { score2++; }
int Board::getScore1() const { return score1; }
int Board::getScore2() const { return score2; }

void Board::resetGame() {
    balls.clear();
    balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);
    paddles.clear();
    paddles.emplace_back((int)((float)(max_width-min_width) / 2 - (0.15 * (max_width-min_width)) / 2), min_height + 5, *this);
    score1 = 0;
    score2 = 0;
}

void Board::setAI1Enabled(bool enabled) {
    ai1_enabled = enabled;
}

void Board::setAI2Enabled(bool enabled) {
    ai2_enabled = enabled;
}

bool Board::getAI2Enabled() {
    return ai2_enabled;
}

void Board::setWireless(bool enabled) {
    wireless = enabled;
}

bool Board::getWireless() {
    return wireless;
}

// BALL OBJECT METHODS

Ball::Ball(float x, float y) : x(x), y(y) {
    radius = 3;
    y_speed = 0;
    while (abs(y_speed) < 0.8) { y_speed = randBetween(-1.5, 1.5); }
    float sign = randBetween(-0.5,0.5);
    x_speed = sign/abs(sign)*sqrt(abs(pow(randBetween(1.5, 2.5),2)-y_speed*y_speed));
    lastDrawnX = round(x);
    lastDrawnY = round(y);
}
Ball::~Ball() {}

void Ball::draw() {
    LCD.SetTextColor(LCD_COLOR_BLACK);
    LCD.FillCircle(lastDrawnX, lastDrawnY, radius);
    LCD.SetTextColor(LCD_COLOR_WHITE);
    lastDrawnX = round(x);
    lastDrawnY = round(y);
    LCD.FillCircle(lastDrawnX, lastDrawnY, radius);
}
float Ball::getx() { return x; }
float Ball::gety() { return y; }
int Ball::getLastDrawnX() { return lastDrawnX; }
int Ball::getLastDrawnY() { return lastDrawnY; }
void Ball::move(Board& board, bool& delete_ball) {
    x = x + x_speed;
    y = y + y_speed;
    delete_ball = false;
    if (y-radius <= board.getMinHeight()) {
        board.incrementScore2();
        delete_ball = true;
    } else if (y+radius >= board.getMaxHeight()) {
        board.incrementScore1();
        delete_ball = true;
    } else if (x-radius <= board.getMinWidth()) {
        x_speed = abs(x_speed);
        x = abs(x-board.getMinWidth()) + board.getMinWidth();
        x = max(board.getMinWidth()+radius, x);
    } else if (x+radius >= board.getMaxWidth()) {
        x_speed = -abs(x_speed);
        x = board.getMaxWidth() - abs(x-board.getMaxWidth());
        x = min(board.getMaxWidth()-radius, x);
    }

    if (y-radius <= board.paddles[0].getBottom() && x <= board.paddles[0].getRight() && x >= board.paddles[0].getLeft()) {
        y_speed = abs(y_speed)*randBetween(1, 1.05);
        x_speed = x_speed*randBetween(1, 1.05);
        if (abs(x_speed) < 0.5) { x_speed+=randBetween(-0.5, 0.5); }
    } else if (y+radius >= board.paddles[1].getTop() && x <= board.paddles[1].getRight() && x >= board.paddles[1].getLeft()) {
        y_speed = -abs(y_speed)*randBetween(1, 1.05);
        x_speed = x_speed*randBetween(1, 1.05);
        if (abs(x_speed) < 0.5) { x_speed+=randBetween(-0.5, 0.5); }
    }
}

// PADDLE OBJECT METHODS

Paddle::Paddle(int x, int y, Board& board) : x(x), y(y), board(board) {
    height = 5;
    width = 0.15 * (board.getMaxWidth()-board.getMinWidth());
    lastDrawnX = x;
    lastDrawnY = y;
}
Paddle::~Paddle() {}

int Paddle::getLeft() {
    return x;
}
int Paddle::getRight() {
    return x+width;
}
int Paddle::getTop() {
    return y;
}
int Paddle::getBottom() {
    return y+height;
}
void Paddle::draw() {
    // Code to draw the paddle at position (x, y)
    LCD.SetTextColor(LCD_COLOR_BLACK);
    LCD.FillRect(lastDrawnX, lastDrawnY, width, height);
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillRect(x, y, width, height);
    lastDrawnX = x;
    lastDrawnY = y;
}
void Paddle::moveRight() {
    if (x+width <= board.getMaxWidth()) {
        x = x + 0.25*width;
        x = min(x, board.getMaxWidth()-width);
    }
}
void Paddle::moveLeft() {
    if (x > board.getMinWidth()) {
        x = x - 0.25*width;
        x = max(board.getMinWidth(), x);
    }
}
void Paddle::moveTo(int new_x) {
    x = max(board.getMinWidth(), min(new_x, board.getMaxWidth() - width));
}

Board board(0, 20, 240, 320);

// ISRs -----------------------------------

void OnboardButtonISR() {
    
    if (MASTER) {
        if (curr_state == STATE_GAME) {
            spawn_ball_flag = true;
        } else if (curr_state == STATE_PAUSE) {
            board.resetGame();
            board.setWireless(false);
            curr_state = STATE_MENU;
        } else if (curr_state == STATE_MENU) {
            board.setAI1Enabled(true);
            board.setAI2Enabled(true);
            board.setWireless(false);
            curr_state = STATE_GAME;
        }
    }

    // // simple test functionality
    // if (curr_state == STATE_MENU) {
    //     curr_state = STATE_GAME;
    // } else if (curr_state == STATE_GAME) {
    //     curr_state = STATE_PAUSE;
    // } else if (curr_state == STATE_PAUSE) {
    //     curr_state = STATE_MENU;
    //     board.resetGame();
    // }
}

void ExternalButton1ISR() {
    if (curr_state == STATE_GAME) {
        if (MASTER) { board.paddles[0].moveLeft(); }
        else { board.paddles[1].moveLeft(); }
    } else if (curr_state == STATE_MENU) {
        if (MASTER) {
            board.setAI1Enabled(false);
            board.setAI2Enabled(true);
            board.setWireless(false);
            curr_state = STATE_GAME;
        }
    }
}

void ExternalButton2ISR() {
    if (MASTER) {
        if (curr_state == STATE_GAME) {
            curr_state = STATE_PAUSE;
        } else if (curr_state == STATE_PAUSE) {
            curr_state = STATE_GAME;
        } else if (curr_state == STATE_MENU) {
            board.setAI1Enabled(false);
            board.setAI2Enabled(false);
            board.setWireless(false);
            curr_state = STATE_GAME;
        }
    }
}

void ExternalButton3ISR() {
    if (curr_state == STATE_GAME) {
        if (MASTER) { board.paddles[0].moveRight(); }
        else { board.paddles[1].moveRight(); }
    } else if (curr_state == STATE_MENU) {
        if (MASTER) {
        board.setAI1Enabled(false);
        board.setAI2Enabled(false);
        board.setWireless(true);
        curr_state = STATE_GAME;
        }
    }
}

void ExternalButton4ISR() {
    if (curr_state == STATE_GAME && !board.getAI2Enabled() && !board.getWireless()) {
        board.paddles[1].moveLeft();
    }
}

void ExternalButton5ISR() {
    if (!board.getAI2Enabled() && !board.getWireless()) {
        if (curr_state == STATE_GAME) {
            curr_state = STATE_PAUSE;
        } else if (curr_state == STATE_PAUSE) {
            curr_state = STATE_GAME;
        }
    }
}

void ExternalButton6ISR() {
    if (curr_state == STATE_GAME && !board.getAI2Enabled() && !board.getWireless()) {
        board.paddles[1].moveRight();
    }
}

void TickerISR() {
    board.moveBalls();
}

// FSM SET UP ------------------------------

void stateMenu(void);
void statePause(void);
void stateGame();

static void (*state_table[])(void) = {stateMenu, statePause, stateGame};

void initializeSM() {
    curr_state = STATE_MENU;
}

void initializeRF() {
    if (MASTER) {
        master.powerUp();
        master.setReceiveMode();
        master.enable();
    } else {
        slave.powerUp();
        slave.setReceiveMode();
        slave.enable();
    }
    logRfDiagnostics();
}

// HELPER FUNCTIONS ------------------------

float min(float a, float b) {
    return a > b ? b : a;
}

float max(float a, float b) {
    return a < b ? b : a;
}

float randBetween(float min, float max) {
    return ((float)(rngGetRandomNumber() % 1000000))/1000000.0 * (max-min)+min;
}

void rngInit() {
    RCC_AHB2ENR |= RCC_AHB2ENR_RNGEN;   // Enables RNG clock
    wait_us(100);                       // Small delay to ensure clock stablity
    RNG_CR |= RNG_CR_RNGEN;             // Enables RNG peripheral
}

uint32_t rngGetRandomNumber() {
    while (!(RNG_SR & RNG_SR_DRDY)) { }         // Wait for a new random number to be avaliable
    if (RNG_SR & (RNG_SR_SEIS | RNG_SR_CEIS)) { // If there is a seed or clock error it turns the RNG off and back on
        RNG_CR &= ~RNG_CR_RNGEN;
        RNG_CR |= RNG_CR_RNGEN;
        return 0;
    }
    return RNG_DR;  // Returns the random 32-bit number from the RNG_DR register
}

void logRfDiagnostics() {
    printf("[Master] Frequency    : %d MHz\n", master.getRfFrequency());
    printf("[Master] Output power : %d dBm\n", master.getRfOutputPower());
    printf("[Master] Data rate    : %d kbps\n", master.getAirDataRate());
    printf("[Master] TX Address   : 0x%llx\n", master.getTxAddress());
    printf("[Master] RX Address   : 0x%llx\n", master.getRxAddress());
}

// STATE FUNCTIONS ---------------------------

void stateMenu() {
    if (prev_state != curr_state) {
        LCD.Clear(LCD_COLOR_BLACK);
        LCD.SetTextColor(LCD_COLOR_WHITE);
        LCD.SetBackColor(LCD_COLOR_BLACK);
        LCD.SetFont(&Font16);
        LCD.DisplayStringAt(0, 80, (uint8_t *)"WELCOME TO PONG", CENTER_MODE);
        LCD.SetFont(&Font12);
        LCD.DisplayStringAt(0, 110, (uint8_t *)"OBB - AI vs AI", CENTER_MODE);
        LCD.DisplayStringAt(0, 130, (uint8_t *)"1 - Human vs AI", CENTER_MODE);
        LCD.DisplayStringAt(0, 150, (uint8_t *)"2 - Human vs Human (Local)", CENTER_MODE);
        LCD.DisplayStringAt(0, 170, (uint8_t *)"3 - Human vs Human (Wireless)", CENTER_MODE);
        prev_state = curr_state;

        if (!MASTER) {
            board.setAI1Enabled(true);
            board.setAI2Enabled(true);
            board.setWireless(true);
        }
    }

    if (!MASTER) {
        board.processIncomingMasterMessage(true);
    }
}

void statePause() {
    if (prev_state != curr_state) {
        LCD.Clear(LCD_COLOR_BLACK);
        prev_state = curr_state;

        // Show pause screen
        LCD.SetTextColor(LCD_COLOR_WHITE);
        LCD.SetBackColor(LCD_COLOR_BLACK);
        LCD.SetFont(&Font16);
        LCD.DisplayStringAt(0, 80, (uint8_t *)"PAUSED", CENTER_MODE);
        LCD.SetFont(&Font12);
        LCD.DisplayStringAt(0, 100, (uint8_t *)"Press 2 to Resume", CENTER_MODE);
        LCD.DisplayStringAt(0, 120, (uint8_t *)"Press OBB to Quit", CENTER_MODE);
        LCD.SetBackColor(LCD_COLOR_WHITE);
        game_ticker.detach();
    }

    if (!MASTER) {
        board.processIncomingMasterMessage(true);
    } else if (board.getWireless()) {
        board.transmitBoardState(true);
    }
}

void stateGame() {
    if (prev_state != curr_state) {
        LCD.Clear(LCD_COLOR_BLACK);
        if (MASTER) { game_ticker.attach(&TickerISR, TICKERTIME); }
        if (board.getWireless()) { initializeRF(); }
        prev_state = curr_state;
    }

    if (!MASTER) {
        board.processIncomingMasterMessage(true);
        board.transmitOutboundSlaveMessage(true);
    }

    // Draw the scoreboard
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillRect(board.getMaxWidth()-board.getMinWidth(), 0, board.getMaxWidth()-board.getMinWidth(), board.getMinHeight());
    LCD.SetTextColor(LCD_COLOR_BLACK);
    LCD.SetBackColor(LCD_COLOR_WHITE);
    LCD.SetFont(&Font12);
    char score_str[30];
    int score1 = board.getScore1();
    int score2 = board.getScore2();
    sprintf(score_str, "(P1) %d - %d (P2)", score1, score2);
    LCD.DisplayStringAt(0, board.getMinHeight()/2-4, (uint8_t *)score_str, CENTER_MODE);

    // Transmit board state and process incoming message
    if (MASTER && board.getWireless()) {
        board.transmitBoardState(true);
        board.processIncomingSlaveMessage(true);
    }

    // Draw the board and paddles
    if (spawn_ball_flag) {
        board.spawnBall();
        spawn_ball_flag = false;
    }
    board.drawBalls();
    board.paddles[0].draw();
    board.paddles[1].draw();
}

// MAIN FUNCTION -----------------------------

int main() {
    onboard_button.fall(&OnboardButtonISR);
    external_button1.attach(&ExternalButton1ISR, IRQ_FALL, 50, false);
    external_button2.attach(&ExternalButton2ISR, IRQ_FALL, 50, false);
    external_button3.attach(&ExternalButton3ISR, IRQ_FALL, 50, false);
    external_button4.attach(&ExternalButton4ISR, IRQ_FALL, 50, false);
    external_button5.attach(&ExternalButton5ISR, IRQ_FALL, 50, false);
    external_button6.attach(&ExternalButton6ISR, IRQ_FALL, 50, false);
    initializeSM();
    while (1) {
        state_table[curr_state]();
        ThisThread::sleep_for(20ms);
    }
}