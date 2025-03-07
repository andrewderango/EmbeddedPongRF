#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "mbed.h"
#include <vector>

// Forward Declarations
class Ball;
class Board;
class Paddle;

// Board Class
class Board {
private:
    int height;
    int width;
    std::vector<Ball> balls;
public:
    Board(int height, int width);
    ~Board();
    int getHeight() const;
    int getWidth() const;
    void drawBalls();
};

// Ball Class
class Ball {
private:
    int x;
    int y;
    int radius;
public:
    Ball(int x, int y);
    ~Ball();
    void draw();
    void move();
};

// Paddle Class
class Paddle {
private:
    int x;
    int y;
    int height;
    int width;
    Board& board;
public:
    Paddle(int x, int y, Board& board);
    ~Paddle();
    void draw();
    void moveRight();
    void moveLeft();
};

// Interrupt Service Routines
void ExternalButton1ISR();
void ExternalButton2ISR();
void ExternalButton3ISR();
void OnboardButtonISR();

// State Machine Setup
void stateMenu();
void statePause();
void stateGame();
void initializeSM();

#endif // FUNCTION_H