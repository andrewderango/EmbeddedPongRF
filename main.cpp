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
  STATE_IDLE = 0,
  STATE_RECORD_TIME,
  STATE_DISPLAY_TIME,
  STATE_CHANGE_TIME
} State_Type;

// GLOBAL VARS ----------------------------

static State_Type curr_state;
static State_Type prev_state;

// ISRs -----------------------------------

void ExternalButton1ISR() {

}

void OnboardButtonISR() {

}

// FSM SET UP ------------------------------

void stateIdle(void);
void stateRecordTime(void);
void stateDisplayTime(void);
void stateChangeTime(void);

static void (*state_table[])(void) = {stateIdle, stateRecordTime, stateDisplayTime, stateChangeTime};

void initializeSM() {
  curr_state = STATE_IDLE;
  prev_state = STATE_DISPLAY_TIME;
}

// HELPER FUNCTIONS ------------------------



// STATE FUNCTIONS ---------------------------



// MAIN FUNCTION -----------------------------

int main() {

  while (1) {
    state_table[curr_state]();
    thread_sleep_for(100);
  }
}