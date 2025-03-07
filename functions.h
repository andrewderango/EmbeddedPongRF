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
    int min_height;
    int max_height;
    int min_width;
    int max_width;
    std::vector<Ball> balls;
    int score1;
    int score2;
public:
    Board(int min_width, int min_height, int max_width, int max_height);
    ~Board();
    int getMinHeight() const;
    int getMinWidth() const;
    int getMaxHeight() const;
    int getMaxWidth() const;
    void drawBalls();
    void moveBalls();
    void incrementScore1();
    void incrementScore2();
    int getScore1() const;
    int getScore2() const;
};

// Ball Class
class Ball {
private:
    int x;
    int y;
    int radius;
    float x_speed;
    float y_speed;
public:
    Ball(int x, int y);
    ~Ball();
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