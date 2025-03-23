# Serial Protocol Documentation

This document describes the serial protocol used for wireless communication in the Pong Embedded System. The protocol is designed to send the number of balls, the position of each ball, the position of each paddle, the score of each team, and the game state.

## Master Message Format

The message is structured as follows:

| Byte Index | Description                        |
|------------|------------------------------------|
| 0          | Number of balls (1 byte)           |
| 1-24       | Positions of balls (3 bytes each)  |
| 25         | Position of Paddle 1 (1 byte)      |
| 26         | Position of Paddle 2 (1 byte)      |
| 27         | Score of Team 1 (1 byte)           |
| 28         | Score of Team 2 (1 byte)           |
| 29         | Game State (1 byte)                |

### Detailed Byte Breakdown

1. **Number of Balls (1 byte)**
   - This byte indicates the number of balls currently in play. The maximum value is 8.

2. **Positions of Balls (24 bytes)**
   - Each ball's position is represented by 3 bytes (1 byte for X coordinate and 2 bytes for Y coordinate).
   - The first ball's position is stored in bytes 1 to 3, the second ball's position in bytes 4 to 6, and so on.
   - If there are fewer than 8 balls, the remaining bytes are set to 0.

3. **Position of Paddle 1 (1 byte)**
   - The X position of Paddle 1 is represented by 1 byte.

4. **Position of Paddle 2 (1 byte)**
   - The X position of Paddle 2 is represented by 1 byte.

5. **Score of Team 1 (1 byte)**
   - The score of Team 1 is represented by 1 byte.

6. **Score of Team 2 (1 byte)**
   - The score of Team 2 is represented by 1 byte.

7. **Game State (1 byte)**
   - This byte indicates the current state of the game:
     - 0: Menu
     - 1: Pause
     - 2: Game

### Example Message

For example, if there are 2 balls with positions (10, 20) and (30, 40), Paddle 1 at position 50, Paddle 2 at position 60, Team 1 score is 5, Team 2 score is 3, and the game is in the "Game" state, the message would be:

| Byte Index | Value |
|------------|-------|
| 0          | 2     |
| 1          | 10    |
| 2-3        | 20    |
| 4          | 30    |
| 5-6        | 40    |
| 7-24       | 0     |
| 25         | 50    |
| 26         | 60    |
| 27         | 5     |
| 28         | 3     |
| 29         | 2     |

This message is then transmitted over the wireless communication channel.

## Notes

- All values are transmitted as unsigned integers in little-endian format.
- Ensure that the receiving end correctly interprets the byte positions and values.