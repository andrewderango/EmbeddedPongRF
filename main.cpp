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
Board::Board(int height, int width) : height(height), width(width) {
    balls.emplace_back(50, 50);
}

// Destructor
Board::~Board() {}

int Board::getHeight() const {return height;}
int Board::getWidth() const {return width;}
void Board::drawBalls() {
    for (int i = 0; i < balls.size(); i++) {
        balls[i].draw();
    }
}

// BALL OBJECT METHODS

Ball::Ball(int x, int y) : x(x), y(y) {
    radius = 3;
}
Ball::~Ball() {}

void Ball::draw() {
    // Code to draw the ball at position (x, y)
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillCircle(x, y, radius);
}
void Ball::move() {
    // Code to move the ball
}

// PADDLE OBJECT METHODS

Paddle::Paddle(int x, int y, Board& board) : x(x), y(y), board(board) {
            height = 5;
            width = 0.15 * board.getWidth();
        }
Paddle::~Paddle() {}

void Paddle::draw() {
    // Code to draw the paddle at position (x, y)
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillRect(x, y, width, height);
}
void Paddle::moveRight() {
    if (x+width <= board.getWidth()) {
        x = x + 0.25*width;
        x = min(x, board.getWidth()-width);
    }
}
void Paddle::moveLeft() {
    if (x > 0) {
        x = x - 0.25*width;
        x = max(0, x);
    }
}

Board board(320, 240);
Paddle paddle1(30, 30, board);
Paddle paddle2(30, 200, board);

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
    //board.draw();
    board.drawBalls();
    paddle1.draw();
    paddle2.draw();
}

// MAIN FUNCTION -----------------------------

int main() {
    onboard_button.fall(&OnboardButtonISR);
    external_button1.attach(&ExternalButton1ISR, IRQ_FALL, 50, false);
    external_button2.attach(&ExternalButton2ISR, IRQ_FALL, 50, false);
    external_button3.attach(&ExternalButton3ISR, IRQ_FALL, 50, false);
    initializeSM();
    while (1) {
        state_table[curr_state]();
        thread_sleep_for(100);
    }
}