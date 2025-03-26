#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "mbed.h"
#include <vector>

// Forward Declarations
class Ball;
class Paddle;
class Board;

// Board Class
class Board {
private:
    int min_height;
    int max_height;
    int min_width;
    int max_width;
    std::vector<Ball> balls;
    int maxNumOfBalls = 8;
    int score1;
    int score2;
    bool ai1_enabled;
    bool ai2_enabled;
    bool wireless;
public:
    Board(int min_width, int min_height, int max_width, int max_height);
    ~Board();
    int getMinHeight() const;
    int getMinWidth() const;
    int getMaxHeight() const;
    int getMaxWidth() const;
    void spawnBall();
    void drawBalls();
    void moveBalls();
    void incrementScore1();
    void incrementScore2();
    int getScore1() const;
    int getScore2() const;
    void resetGame();
    void setAI1Enabled(bool enabled);
    void setAI2Enabled(bool enabled);
    bool getAI2Enabled();
    void setWireless(bool enabled);
    bool getWireless();
    std::vector<Paddle> paddles;
    int transmitBoardState(bool verbose);
    int processIncomingSlaveMessage(bool verbose);
    int processIncomingMasterMessage(bool verbose);
    int transmitOutboundSlaveMessage(bool verbose);
};

// Ball Class
class Ball {
private:
    float x;
    float y;
    int radius;
    float x_speed;
    float y_speed;
    int lastDrawnX;
    int lastDrawnY;
public:
    Ball(float x, float y);
    ~Ball();
    float getx();
    float gety();
    int getLastDrawnX();
    int getLastDrawnY();
    void draw();
    void move(Board& board, bool& del);
};

// Paddle Class
class Paddle {
private:
    int x;
    int y;
    int height;
    int width;
    int lastDrawnX;
    int lastDrawnY;
    Board& board;
public:
    Paddle(int x, int y, Board& board);
    ~Paddle();
    int getLeft();
    int getRight();
    int getTop();
    int getBottom();
    void draw();
    void moveRight();
    void moveLeft();
    void moveTo(int new_x);
};

// Interrupt Service Routines
void ExternalButton1ISR();
void ExternalButton2ISR();
void ExternalButton3ISR();
void ExternalButton4ISR();
void ExternalButton5ISR();
void ExternalButton6ISR();
void OnboardButtonISR();
void TickerISR();
void GoalTickerCallback();

// State Machine Setup
void stateMenu();
void statePause();
void stateGame();
void initializeSM();
void initializeRF();

// Helper Functions
float min(float a, float b);
float max(float a, float b);
float randBetween(float min, float max);
void rngInit();
uint32_t rngGetRandomNumber();
void logRfDiagnostics();

#endif // FUNCTION_H