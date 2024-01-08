#include <Arduino.h>

// SSD1306 Interfacing libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// Graphics libraries
#include <Adafruit_GFX.h>
// Custom fireworks animation library
#include <fireworks_ssd1306.h>

// Pin definitions
#define UP_BUTTON       6
#define DOWN_BUTTON     7

// Screen definition (w/h in pixels, reset pin -1 -> same as Arduino)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1

// Function definitions
bool refreshBall(unsigned long time);
bool refreshPaddles(unsigned long time);
void goal(String winner);
void victoryScreen(String winner);
void renderMenu();

// Game variables
const unsigned int WIN_SCORE =               5; // Score required to win a match
const unsigned long PADDLE_UPDATE_DELAY =    1; // Delay between paddle updates (ms)
const unsigned long BALL_UPDATE_DELAY =      1; // Delay between ball updates (ms)
const uint8_t PADDLE_LENGTH =               12; // Length of both paddles
unsigned int DIFFICULTY =                   30; // CPU paddle difficulty setting (min 0, max 127)
bool gameState =                         false; // Game state variable for menu implementation

// Ball variables
uint8_t ball_x =    64, ball_y =    32; // Position
uint8_t ball_dir_x = 1, ball_dir_y = 1; // Direction
uint8_t new_x, new_y;                   // Updated position placeholders
unsigned long ball_update;

// CPU Paddle variables
unsigned long paddle_update;
const uint8_t CPU_X = 12;
uint8_t cpu_y =       16;

// Player Paddle variables
const uint8_t PLAYER_X = 115;
uint8_t       player_y =  16;

// Player Control input state booleans
static bool   up_state = false;
static bool down_state = false;

// Half paddle length (constant)
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

    // Enable internal pull-up resistor on input pins
    digitalWrite(UP_BUTTON,1);
    digitalWrite(DOWN_BUTTON,1);

    // 1 second buffer before continuing
    while(millis() - start < 1000);

    // Set paddle and ball refresh counters before beginning game logic
    paddle_update = ball_update = millis();
}

void loop() {
    if (!gameState)
    {
        renderMenu();
    }

    // Reset update and control booleans, refresh real-time counter
    bool update = false;
    unsigned long time = millis();
    
    // Update player control states
    up_state |= (digitalRead(UP_BUTTON) == LOW);
    down_state |= (digitalRead(DOWN_BUTTON) == LOW);

    // Request an update if either the ball or paddles are due for a refresh
    update = refreshBall(time) | refreshPaddles(time);
    // Bitwise OR operator '|' used above since the normal '||' OR operator will
    // simply ignore the second condition to save time, if the first condition returns 'true'

    // Refresh display whenever requested
    if(update && gameState)
    {
        display.display();
    }
}

// Render main menu
void renderMenu()
{
    display.clearDisplay();
    display.drawRect(0, 0, 128, 64, WHITE);

    // Different text style for menu
    display.setTextSize(2);
    // Render the play button in a box
    display.getTextBounds(String("Play"), 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, (SCREEN_HEIGHT-centerheight)/2);
    display.println("Play");
    display.drawRect(((SCREEN_WIDTH-centerwidth)/2) - 5, ((SCREEN_HEIGHT-centerheight)/2) - 5, centerwidth + 10, centerheight + 10, WHITE);
    // Render help text
    display.setTextSize(1);
    display.getTextBounds(String("[press any button]"), 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, (SCREEN_HEIGHT/2)+20);
    display.println("[press any button]");

    display.display();
    
    while (!(digitalRead(UP_BUTTON) == LOW) && !(digitalRead(DOWN_BUTTON) == LOW))
    {
        // do absolutely nothing
    }

    // Invert the play button colors for a second as a reaction (same code, opposite colors)
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.getTextBounds(String("Play"), 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.fillRect(((SCREEN_WIDTH-centerwidth)/2) - 5, ((SCREEN_HEIGHT-centerheight)/2) - 5, centerwidth + 10, centerheight + 10, WHITE);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, (SCREEN_HEIGHT-centerheight)/2);
    display.println("Play");
    display.display();
    delay(150);

    // Reset display properties
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();

    // Set game state
    gameState = true;

    // Draw court
    display.drawRect(0, 0, 128, 64, WHITE);
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
            goal("PLAYER"); // Player scored a goal
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

        ball_update += BALL_UPDATE_DELAY;
        
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
        // The difficulty setting limits the horizontal proximity in which the CPU can 'see' the ball and move the paddle in response to it (max = 127, min 0)
        // CPU paddle also gives up if the ball is already behind its paddle
        if (ball_x < DIFFICULTY && ball_x >= 12)
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

        paddle_update += PADDLE_UPDATE_DELAY;

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

    // Random direction for the reset ball (-1/1 for both x and y)
    ball_dir_x = (-1 + 2*(rand() % 2)), ball_dir_y = (-1 + 2*(rand() % 2));
    new_x = ball_x + ball_dir_x;
    new_y = ball_y + ball_dir_y;

    // Reset paddles
    player_y = cpu_y = 16;

    // Randomize CPU difficulty
    DIFFICULTY = 12+(rand() % 43);
}

void victoryScreen(String winner)
{
    // IF player won:
    // Pull graphics from fireworks library and run an animation frame by frame
    if (winner != "CPU")
    {
        for (int i = 0; i < e_allArray_LEN; i++)
        {
            display.clearDisplay();
            display.drawRect(0, 0, 128, 64, WHITE);
            display.drawBitmap(0, 0, e_allArray[i], SCREEN_WIDTH, SCREEN_HEIGHT, 1);
            display.display();
            delay(100);
        }
    }
    display.clearDisplay();
    display.drawRect(0, 0, 128, 64, WHITE);

    // Animation and scoreboard display
    display.getTextBounds(String(winner + " WINS!"), 0, 0, &centercursorx, &centercursory, &centerwidth, &centerheight);
    display.setCursor((SCREEN_WIDTH-centerwidth)/2, (SCREEN_HEIGHT-centerheight)/2);
    display.println(winner + " WINS!");
    display.display();
    delay(2000);

    // Reset game state to send player back to menu
    gameState = false;
}