#include <Arduino.h>

// SSD1306 Interfacing libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// Graphics libraries
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoOblique9pt7b.h>
// Custom fireworks animation library
#include <fireworks_ssd1306.h>

// Pin definitions
#define UP_BUTTON       6
#define DOWN_BUTTON     7
// Screen definition (w/h in pixels, reset pin -1 -> same as Arduino)
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1

// Function definitions
bool refreshBall(unsigned long time);
bool refreshPaddles(unsigned long time);
void goal(String winner);
void victoryScreen(String winner);
void mainMenu();

// Game variables
const unsigned int WIN_SCORE =               5;
const unsigned long PADDLE_UPDATE_RATE =    20;
const unsigned long BALL_UPDATE_RATE =      5;
const uint8_t PADDLE_LENGTH =               16;
bool gameState =                         false;

// Ball variables
uint8_t ball_x = 64, ball_y = 32; // Position
uint8_t ball_dir_x = 1, ball_dir_y = 1; // Direction
uint8_t new_x, new_y; // Updated position placeholders
unsigned long ball_update;

// CPU Paddle variables
unsigned long paddle_update;
const uint8_t CPU_X = 12;
uint8_t cpu_y = 16;

// Player Paddle and control variables
const uint8_t PLAYER_X = 115;
uint8_t player_y = 16;
static bool up_state = false;
static bool down_state = false;

// Half paddle length constant
const uint8_t half_paddle = PADDLE_LENGTH / 2;

// Setup text centering value storage
int16_t centercursorx, centercursory; uint16_t centerwidth, centerheight;

// Scorekeeping
unsigned int cpu_score, player_score = 0;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    // Initialize display, splash adafruit logo briefly
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    unsigned long start = millis();
    delay(500);
    display.clearDisplay();
    display.display();

    // Setup text rendering
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setTextWrap(false);

    // Input pin assignment
    pinMode(UP_BUTTON, INPUT);
    pinMode(DOWN_BUTTON, INPUT);

    // Enable internal pullup resistor on input pins (not necessary with 10kOhm resistors on the circuit already)
    // digitalWrite(UP_BUTTON,1);
    // digitalWrite(DOWN_BUTTON,1);

    // Draw court
    display.drawRect(0, 0, 128, 64, WHITE);

    // Start game after 1 second has passed
    while(millis() - start < 1000);
    display.display();

    paddle_update = ball_update = millis();
}

void loop() {
    // Reset update and control booleans, refresh real-time counter
    bool update = false;
    unsigned long time = millis();
    
    // Update player control states
    up_state |= (digitalRead(UP_BUTTON) == LOW);
    down_state |= (digitalRead(DOWN_BUTTON) == LOW);

    // Request an update if either the ball or paddles are due for a refresh
    update = refreshBall(time) | refreshPaddles(time);
    // Bitwise OR operator '|' used above since the normal '||' OR operator will simply ignore the second condition to save time, if the first condition returns 'true'

    // Refresh display whenever requested
    if(update)
    {
        display.display();
    }
}

// Update ball location
bool refreshBall(unsigned long time)
{
    if(time > ball_update)
    {
        // Move ball to next location
        new_x = ball_x + ball_dir_x;
        new_y = ball_y + ball_dir_y;

        // Check for a vertical wall collision, consequently call a goal
        if(new_x == 0)
        {
            goal("USER"); // Player scored a goal
        }
        else if(new_x == 127)
        {
            goal("CPU"); // CPU scored a goal
        }

        // Logic for paddle and top/bottom wall collisions:
        // Inverse the direction of the ball in x or y directions respectively, maintain the other direction

        // Check for a horizontal wall collision
        if(new_y == 0 || new_y == 63)
        {
            ball_dir_y = -ball_dir_y;
            new_y += ball_dir_y + ball_dir_y;
        }

        // Check for a CPU Paddle collision
        if(new_x == CPU_X && new_y >= cpu_y && new_y <= cpu_y + PADDLE_LENGTH)
        {
            ball_dir_x = -ball_dir_x;
            new_x += ball_dir_x + ball_dir_x;
        }

        // Check for a Player Paddle collision
        if(new_x == PLAYER_X && new_y >= player_y && new_y <= player_y + PADDLE_LENGTH)
        {
            ball_dir_x = -ball_dir_x;
            new_x += ball_dir_x + ball_dir_x;
        }

        // Clear old ball and draw new ball on the updated location
        display.drawPixel(ball_x, ball_y, BLACK);
        display.drawPixel(new_x, new_y, WHITE);
        ball_x = new_x;
        ball_y = new_y;

        ball_update += BALL_UPDATE_RATE;
        
        // Set update boolean to true to force display update
        return true;
    }
    return false;
}

// Update paddle positions
bool refreshPaddles(unsigned long time)
{
    if (time > paddle_update)
    {
        // Clear old CPU Paddle
        display.drawFastVLine(CPU_X, cpu_y, PADDLE_LENGTH, BLACK);
        // Move CPU Paddle based on the y coordinate of the ball
        if (ball_x < 30) // Modification: gimping the CPU paddle by making it only move once the ball is on its own closest 30 pixels of the court
        {
            if(cpu_y + half_paddle > ball_y) {
                cpu_y -= 1;
            }
            if(cpu_y + half_paddle < ball_y) {
                cpu_y += 1;
            }
        }
        // Boundary implementation
        if(cpu_y < 1) cpu_y = 1;
        if(cpu_y + PADDLE_LENGTH > 63) cpu_y = 63 - PADDLE_LENGTH;
        // Draw new CPU Paddle
        display.drawFastVLine(CPU_X, cpu_y, PADDLE_LENGTH, WHITE);

        // Clear old Player Paddle
        display.drawFastVLine(PLAYER_X, player_y, PADDLE_LENGTH, BLACK);
        // Move Player Paddle based on the control state
        if(up_state) {
            player_y -= 1;
        }
        if(down_state) {
            player_y += 1;
        }
        // Reset input state variables
        up_state = down_state = false;
        // Boundary implementation
        if(player_y < 1) player_y = 1;
        if(player_y + PADDLE_LENGTH > 63) player_y = 63 - PADDLE_LENGTH;
        // Draw new CPU Paddle
        display.drawFastVLine(PLAYER_X, player_y, PADDLE_LENGTH, WHITE);

        paddle_update += PADDLE_UPDATE_RATE;

        // Set update boolean to true to force display update
        return true;
    }
    return false;
}

// Goal scorekeeping and celebration screen
void goal(String winner)
{
    // Clear court area
    display.fillRect(1, 1, 126, 62, BLACK);
    if (winner != "CPU")
    {
        // Player goal
        player_score += 1;
    }
    else
    {
        // CPU goal
        cpu_score += 1;
    }

    // Animation and scoreboard display
    display.getTextBounds(String(winner + " SCORES!"), 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, ((SCREEN_HEIGHT-centerheight)/2) - 10);
    display.println(winner + " SCORES!");
    String scoreboard = "[CPU " + String(cpu_score) + " : " + String(player_score) + " PLAYER]";
    display.getTextBounds(scoreboard, 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, ((SCREEN_HEIGHT-centerheight)/2) + 8);
    display.println(scoreboard);
    display.display();
    delay(2000);
    display.clearDisplay();
    // If score passes some max value, display cooler animation and offer a replay
    if (player_score >= WIN_SCORE || cpu_score >= WIN_SCORE)
    {
        victoryScreen(winner);
        player_score = cpu_score = 0;
    }
    // Reset court and ball
    display.clearDisplay();
    display.drawRect(0, 0, 128, 64, WHITE);
    ball_x = 64, ball_y = 32;
    // Random direction for the reset ball
    ball_dir_x = (-1 + 2*(rand() % 2)), ball_dir_y = (-1 + 2*(rand() % 2));
    new_x = ball_x + ball_dir_x;
    new_y = ball_y + ball_dir_y;
    // Reset paddles
    player_y = cpu_y = 16;
}

void victoryScreen(String winner)
{
    // Pull graphics from fireworks library and run an animation
    for (int i = 0; i < e_allArray_LEN; i++)
    {
        display.clearDisplay();
        display.drawBitmap(0, 0, e_allArray[i], SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        display.display();
        delay(100);
    }
    display.clearDisplay();

    // Different text style for victory screen
    display.setFont(&FreeMonoOblique9pt7b);

    // Animation and scoreboard display
    display.getTextBounds(String(winner + " WINS!"), 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, SCREEN_HEIGHT/2);
    display.println(winner + " WINS!");
    /* String scoreboard = "[CPU " + String(cpu_score) + " : " + String(player_score) + " PLAYER]";
    display.getTextBounds(scoreboard, 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, ((SCREEN_HEIGHT-centerheight)/2) + 8);
    display.println(scoreboard); */
    display.display();
    delay(2000);
    // Different text style for victory screen
    display.setFont(NULL);
}