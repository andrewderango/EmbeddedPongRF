#include "LCD_DISCO_F429ZI.h"
#include "DebouncedInterrupt.h"
#include "mbed.h"
#include <time.h>

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



// ISRs -----------------------------------

void ExternalButton1ISR() {

}

void ExternalButton2ISR() {

}

void ExternalButton3ISR() {

}

void OnboardButtonISR() {
    if (curr_state == STATE_MENU) {
        curr_state = STATE_GAME;
    } else if (curr_state == STATE_GAME) {
        curr_state = STATE_PAUSE;
    } else {
        curr_state = STATE_GAME;
    }
}

// FSM SET UP ------------------------------

void stateMenu(void);
void statePause(void);
void stateGame(void);

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
}

// MAIN FUNCTION -----------------------------

int main() {
    onboard_button.fall(&OnboardButtonISR);
    initializeSM();
    while (1) {
        state_table[curr_state]();
        thread_sleep_for(100);
    }
}