# Serial Protocol Documentation

This document describes the serial protocol used for wireless communication in the Pong Embedded System. The protocol is designed to send the number of balls, the position of each ball, the position of each paddle, and the score of each team.

## Master Message Format

The message is structured as follows:

| Byte Index | Description                        |
|------------|------------------------------------|
| 0          | Number of balls (1 byte)           |
| 1-20       | Positions of balls (4 bytes each)  |
| 21         | Position of Paddle 1 (1 byte)      |
| 22         | Position of Paddle 2 (1 byte)      |
| 23-24      | Score of Team 1 (2 bytes)          |
| 25-26      | Score of Team 2 (2 bytes)          |
| 27         | Stop byte (1 byte)                 |

### Detailed Byte Breakdown

1. **Number of Balls (1 byte)**
   - This byte indicates the number of balls currently in play. The maximum value is 5.

2. **Positions of Balls (20 bytes)**
   - Each ball's position is represented by 4 bytes (2 bytes for X coordinate and 2 bytes for Y coordinate).
   - The first ball's position is stored in bytes 1 to 4, the second ball's position in bytes 5 to 8, and so on.
   - If there are fewer than 5 balls, the remaining bytes are set to 0.

3. **Position of Paddle 1 (1 byte)**
   - The X position of Paddle 1 is represented by 1 byte.

4. **Position of Paddle 2 (1 byte)**
   - The X position of Paddle 2 is represented by 1 byte.

5. **Score of Team 1 (2 bytes)**
   - The score of Team 1 is represented by 2 bytes.

6. **Score of Team 2 (2 bytes)**
   - The score of Team 2 is represented by 2 bytes.

7. **Stop Byte (1 byte)**
   - This byte is set to `0x00` to indicate the end of the message.

### Example Message

For example, if there are 2 balls with positions (10, 20) and (30, 40), Paddle 1 at position 50, Paddle 2 at position 60, Team 1 score is 5, and Team 2 score is 3, the message would be:

| Byte Index | Value |
|------------|-------|
| 0          | 2     |
| 1-2        | 10    |
| 3-4        | 20    |
| 5-6        | 30    |
| 7-8        | 40    |
| 9-20       | 0     |
| 21         | 50    |
| 22         | 60    |
| 23-24      | 5     |
| 25-26      | 3     |
| 27         | 0     |

This message is then transmitted over the wireless communication channel.

## Notes

- All values are transmitted as unsigned integers in little-endian format.
- Ensure that the receiving end correctly interprets the byte positions and values.