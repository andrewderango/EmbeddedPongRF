#include "functions.h"
#include "LCD_DISCO_F429ZI.h"
#include "DebouncedInterrupt.h"
#include "mbed.h"
#include <time.h>
#include <vector>
#include <cstdlib>

#define SDA_PIN PC_9
#define SCL_PIN PA_8
#define TICKERTIME 20ms
// Enables the AI, and difficulty should be between 1-10 (Easy-Hard)
#define AI1_ENABLED 0
#define AI1_DIFFICULTY 5
#define AI2_ENABLED 1
#define AI2_DIFFICULTY 1

// DEVICES --------------------------------

LCD_DISCO_F429ZI LCD;
I2C i2c(SDA_PIN, SCL_PIN);
DigitalOut led(PG_14);
Ticker game_ticker;

// INTERRUPTS -----------------------------

InterruptIn onboard_button(BUTTON1);
DebouncedInterrupt external_button1(PA_5);
DebouncedInterrupt external_button2(PA_6);
DebouncedInterrupt external_button3(PA_7);

// DATA TYPES -----------------------------

typedef enum {
  STATE_MENU = 0,
  STATE_PAUSE = 1,
  STATE_GAME = 2,
} State_Type;

// GLOBAL VARS ----------------------------

static State_Type curr_state;
static State_Type prev_state = STATE_GAME;

// OBJECTS --------------------------------
// BOARD OBJECT METHODS
        
// Constructor
Board::Board(int min_width, int min_height, int max_width, int max_height) : min_width(min_width), min_height(min_height), max_width(max_width), max_height(max_height) {
    balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);
    paddles.emplace_back((int)((float)(max_width-min_width) / 2 - (0.15 * (max_width-min_width)) / 2), min_height + 5, *this);
    paddles.emplace_back((int)((float)(max_width-min_width) / 2 - (0.15 * (max_width-min_width)) / 2), max_height - 10, *this);
    score1 = 0;
    score2 = 0;
}

// Destructor
Board::~Board() {}

int Board::getMinHeight() const {return min_height;}
int Board::getMinWidth() const {return min_width;}
int Board::getMaxHeight() const {return max_height;}
int Board::getMaxWidth() const {return max_width;}
void Board::drawBalls() {
    for (int i = 0; i < balls.size(); i++) {
        balls[i].draw();
    }
}
void Board::moveBalls() {
    for (int i = 0; i < balls.size(); i++) {
        bool del = false;
        balls[i].move(*this, del);
        if (del) {
            LCD.SetTextColor(LCD_COLOR_BLACK);
            LCD.FillCircle(balls[i].getLastDrawnX(), balls[i].getLastDrawnY(), 3);
            // LCD.FillCircle(balls[i].getx(), balls[i].gety(), 3);
            balls.erase(balls.begin() + i);
            i--;
        }
    }
    if (balls.size() <= 0) {balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);}
    
    // AI opponent
    float rand_num = randBetween(0,11);
    if (balls[0].getx() < paddles[0].getLeft() && AI1_ENABLED && rand_num < AI1_DIFFICULTY) {
        paddles[0].moveLeft();
    } else if (balls[0].getx() > paddles[0].getRight() && AI1_ENABLED && rand_num < AI1_DIFFICULTY) {
        paddles[0].moveRight();
    }
    if (balls[0].getx() < paddles[1].getLeft() && AI2_ENABLED && rand_num > 11-AI2_DIFFICULTY) {
        paddles[1].moveLeft();
    } else if (balls[0].getx() > paddles[1].getRight() && AI2_ENABLED && rand_num > 11-AI2_DIFFICULTY) {
        paddles[1].moveRight();
    }
}
void Board::incrementScore1() {score1++;}
void Board::incrementScore2() {score2++;}
int Board::getScore1() const {return score1;}
int Board::getScore2() const {return score2;}

void Board::resetGame() {
    balls.clear();
    balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);
    paddles.clear();
    paddles.emplace_back((int)((float)(max_width-min_width) / 2 - (0.15 * (max_width-min_width)) / 2), min_height + 5, *this);
    score1 = 0;
    score2 = 0;
}

// BALL OBJECT METHODS

Ball::Ball(int x, int y) : x(x), y(y) {
    radius = 3;
    x_speed = round(randBetween(-3,3));
    float randy = round(randBetween(-1,1));
    y_speed = randy/abs(randy);
    lastDrawnX = x;
    lastDrawnY = y;
}
Ball::~Ball() {}

void Ball::draw() {
    // Code to draw the ball at position (x, y)
    LCD.SetTextColor(LCD_COLOR_BLACK);
    LCD.FillCircle(lastDrawnX, lastDrawnY, radius);
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillCircle(x, y, radius);
    lastDrawnX = x;
    lastDrawnY = y;
    printf("%d, %d\n", (int)(100*x_speed), (int)(100*y_speed));
}
int Ball::getx() {return x;}
int Ball::gety() {return y;}
int Ball::getLastDrawnX() {return lastDrawnX;}
int Ball::getLastDrawnY() {return lastDrawnY;}
void Ball::move(Board& board, bool& del) {
    x = x + x_speed;
    y = y + y_speed;
    del = false;
    if (y-radius <= board.getMinHeight()) {
        board.incrementScore2();
        del = true;
    } else if (y+radius >= board.getMaxHeight()) {
        board.incrementScore1();
        del = true;
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
        y_speed = abs(y_speed);
    } else if (y+radius >= board.paddles[1].getTop() && x <= board.paddles[1].getRight() && x >= board.paddles[1].getLeft()) {
        y_speed = -abs(y_speed);
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

Board board(0, 20, 240, 320);

// ISRs -----------------------------------

void ExternalButton1ISR() {
    board.paddles[0].moveLeft();
}

void ExternalButton2ISR() {
    if (curr_state == STATE_GAME) {
        curr_state = STATE_PAUSE;
    } else if (curr_state == STATE_PAUSE) {
        curr_state = STATE_GAME;
    }
}

void ExternalButton3ISR() {
    board.paddles[0].moveRight();
}

void OnboardButtonISR() {
    if (curr_state == STATE_MENU) {
        curr_state = STATE_GAME;
    } else if (curr_state == STATE_PAUSE) {
        board.resetGame();
        curr_state = STATE_MENU;
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

// HELPER FUNCTIONS ------------------------

float randBetween(float min, float max) {
    return (float)rand()/(float)INT_MAX*(max-min)+min;
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
        LCD.DisplayStringAt(0, 110, (uint8_t *)"1 - Play Versus AI", CENTER_MODE);
        LCD.DisplayStringAt(0, 130, (uint8_t *)"2 - Play Versus Human", CENTER_MODE);
        prev_state = curr_state;
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
        LCD.SetBackColor(LCD_COLOR_WHITE);
        game_ticker.detach();
    }
}

void stateGame() {
    if (prev_state != curr_state) {
        LCD.Clear(LCD_COLOR_BLACK);
        game_ticker.attach(&TickerISR, TICKERTIME);
        prev_state = curr_state;
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

    // Draw the board and paddles
    // LCD.SetTextColor(LCD_COLOR_BLACK);
    // LCD.FillRect(board.getMinWidth(), board.getMinHeight(), board.getMaxWidth()-board.getMinWidth(), board.getMaxHeight()-board.getMinHeight());
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
    srand(time(NULL));
    initializeSM();
    while (1) {
        state_table[curr_state]();
        thread_sleep_for(20);
    }
}