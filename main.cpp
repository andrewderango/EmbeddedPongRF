#include "functions.h"
#include "LCD_DISCO_F429ZI.h"
#include "DebouncedInterrupt.h"
#include "mbed.h"
#include <time.h>
#include <vector>

#define SDA_PIN PC_9
#define SCL_PIN PA_8

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

// OBJECTS --------------------------------
// BOARD OBJECT METHODS
        
// Constructor
Board::Board(int min_width, int min_height, int max_width, int max_height) : min_width(min_width), min_height(min_height), max_width(max_width), max_height(max_height) {
    balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);
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
            balls.erase(balls.begin() + i);
            i--;
        }
    }
    if (balls.size() <= 0) {balls.emplace_back(min_width+(max_width-min_width)/2, min_height+(max_height-min_height)/2);}
}
void Board::incrementScore1() {score1++;}
void Board::incrementScore2() {score2++;}

// BALL OBJECT METHODS

Ball::Ball(int x, int y) : x(x), y(y) {
    radius = 3;
    x_speed = 1;
    y_speed = -1;
}
Ball::~Ball() {}

void Ball::draw() {
    // Code to draw the ball at position (x, y)
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillCircle(x, y, radius);
}
void Ball::move(Board& board, bool& del) {
    x = x + x_speed;
    y = y + y_speed;
    del = false;
    if (y-radius <= board.getMinHeight()) {
        board.incrementScore1();
        del = true;
    } else if (y+radius >= board.getMaxHeight()) {
        board.incrementScore2();
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
}

// PADDLE OBJECT METHODS

Paddle::Paddle(int x, int y, Board& board) : x(x), y(y), board(board) {
            height = 5;
            width = 0.15 * (board.getMaxWidth()-board.getMinWidth());
        }
Paddle::~Paddle() {}

void Paddle::draw() {
    // Code to draw the paddle at position (x, y)
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillRect(x, y, width, height);
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

Board board(0, 0, 240, 300);
Paddle paddle1((int)((float)(board.getMaxWidth()-board.getMinWidth()) / 2 - (0.15 * (board.getMaxWidth()-board.getMinWidth())) / 2), board.getMinHeight() + 5, board);
Paddle paddle2((int)((float)(board.getMaxWidth()-board.getMinWidth()) / 2 - (0.15 * (board.getMaxWidth()-board.getMinWidth())) / 2), board.getMaxHeight() - 10, board);

// ISRs -----------------------------------

void ExternalButton1ISR() {
    paddle1.moveLeft();
}

void ExternalButton2ISR() {
    if (curr_state == STATE_GAME) {
        curr_state = STATE_PAUSE;
    } else if (curr_state == STATE_PAUSE) {
        curr_state = STATE_GAME;
    }
}

void ExternalButton3ISR() {
    paddle1.moveRight();
}

void OnboardButtonISR() {
    if (curr_state == STATE_MENU) {
        curr_state = STATE_GAME;
    } else if (curr_state == STATE_PAUSE) {
        curr_state = STATE_MENU;
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

// HELPER FUNCTIONS ------------------------



// STATE FUNCTIONS ---------------------------

void stateMenu() {
    LCD.Clear(LCD_COLOR_BLUE);
}

void statePause() {
    LCD.Clear(LCD_COLOR_RED);
}

void stateGame() {
    LCD.Clear(LCD_COLOR_BLACK);
    board.drawBalls();
    paddle1.draw();
    paddle2.draw();
}

// MAIN FUNCTION -----------------------------

int main() {
    game_ticker.attach(&TickerISR, 20ms);
    onboard_button.fall(&OnboardButtonISR);
    external_button1.attach(&ExternalButton1ISR, IRQ_FALL, 50, false);
    external_button2.attach(&ExternalButton2ISR, IRQ_FALL, 50, false);
    external_button3.attach(&ExternalButton3ISR, IRQ_FALL, 50, false);
    initializeSM();
    while (1) {
        state_table[curr_state]();
        thread_sleep_for(20);
    }
}