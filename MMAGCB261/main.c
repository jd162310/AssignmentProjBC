/**************************************************************
 * main.c
 * rev 1.0 20-Mar-2026 bjays
 * MMAGCB261
 * ***********************************************************/


#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// global variables
// Define pins
  #define X_STEP 5
  #define X_DIR 6
  #define Y_STEP 11
  #define Y_DIR 12
  #define Z_STEP 14
  #define Z_DIR 15
  #define ENABLE 22
  // Mode pins for microstepping
  #define X_Mode0 2
  #define X_Mode1 3
  #define X_Mode2 4
  #define Y_Mode0 8
  #define Y_Mode1 9
  #define Y_Mode2 10

// delay variables for pulse timing
int high_delay_us = 1000; // microseconds
int low_delay_us = 1000; // microseconds

// axis selection variable
char axis_selection = 'x'; // default to x-axis

// global variables for buffer
# define buffer_size 100
char command_buffer[buffer_size]; // array to store characters from user input
int buffer_index = 0; // index to keep track of position in buffer
bool command_complete = false; // flag to indicate when a command is complete

// global variable for direction
bool forward = true; // defaults to forward direction

// global variable for mircostepping mode
int mode = 1; // defaults to full step mode

// glodal variable for number of steps
int steps = 0; // defaults to 0 steps

//G-code manual mode variables
float pos_x = 0, pos_y = 0, pos_z = 0;
float steps_per_mm = 100; //placeholder, how many steps = 1mm of movement
float step_size = 0.05; //How many mm to move each time
bool manual_mode = false; // true = manual, false = default

// Key state tracking for manual mode
bool key_w = false; // Y+
bool key_s = false; // Y-
bool key_a = false; // X-
bool key_d = false; // X+
bool key_o = false; // Z+
bool key_p = false; // Z-


// origin point variables
float origin_x = 0;
float origin_y = 0;
float origin_z = 0;
bool origin_set = false; // flag to indicate if origin has been set


// Function for pin initialization
void init_stepper_pins() {
  // set the pins to output
  gpio_init(ENABLE);
  gpio_set_dir(ENABLE, GPIO_OUT);
  gpio_put(ENABLE, 0);
  // X stepper motor pins
  gpio_init(X_STEP);
  gpio_init(X_DIR);
  gpio_set_dir(X_STEP, GPIO_OUT);
  gpio_set_dir(X_DIR, GPIO_OUT);
  gpio_init(X_Mode0);
  gpio_init(X_Mode1);
  gpio_init(X_Mode2);
  gpio_set_dir(X_Mode0, GPIO_OUT);
  gpio_set_dir(X_Mode1, GPIO_OUT);
  gpio_set_dir(X_Mode2, GPIO_OUT);
  // y stepper motor pins
  gpio_init(Y_STEP);
  gpio_init(Y_DIR);
  gpio_set_dir(Y_STEP, GPIO_OUT);
  gpio_set_dir(Y_DIR, GPIO_OUT);
  gpio_init(Y_Mode0);
  gpio_init(Y_Mode1);
  gpio_init(Y_Mode2);
  gpio_set_dir(Y_Mode0, GPIO_OUT);
  gpio_set_dir(Y_Mode1, GPIO_OUT);
  gpio_set_dir(Y_Mode2, GPIO_OUT);
  // z stepper motor pins
  gpio_init(Z_STEP);
  gpio_init(Z_DIR);
  gpio_set_dir(Z_STEP, GPIO_OUT);
  gpio_set_dir(Z_DIR, GPIO_OUT);
}

// Functions to send pulse signal to each stepper motor
void send_pulse_to_stepperx() {
  gpio_put(X_STEP, 1);
  sleep_us(high_delay_us);
  gpio_put(X_STEP, 0);
  sleep_us(low_delay_us);
}
void send_pulse_to_steppery() {
  gpio_put(Y_STEP, 1);
  sleep_us(high_delay_us);
  gpio_put(Y_STEP, 0);
  sleep_us(low_delay_us);
}
void send_pulse_to_stepperz() {
  gpio_put(Z_STEP, 1);
  sleep_us(high_delay_us);
  gpio_put(Z_STEP, 0);
  sleep_us(low_delay_us);
}  

// Function to execute a number of steps
void execute_n_steps() {
  // calculate how many mm moved
  float mm_moved = steps / steps_per_mm;
  if (!forward) 
    mm_moved = -mm_moved; 
  for (int i = 0; i < steps; i++) {
    switch (axis_selection) {
      case 'x':
      case 'X':
        send_pulse_to_stepperx();
        break;
      case 'y':
      case 'Y':
        send_pulse_to_steppery();
        break;
      case 'z':
      case 'Z':
        send_pulse_to_stepperz();
        break;
    }
  }
  switch (axis_selection) {
    case 'x':
    case 'X':
      pos_x += mm_moved;
      break;
    case 'y':
    case 'Y':
      pos_y += mm_moved;
      break;
    case 'z':
    case 'Z':
      pos_z += mm_moved;
      break;
  }
}

// function to set the direction of the stepper motor
void set_stepper_direction() {
  // sets direction forward or backward based on user input
  gpio_put(X_DIR, forward);
  gpio_put(Y_DIR, forward);
  gpio_put(Z_DIR, forward);
}

// function to set the microstepping mode based on user input
void set_microstepping_mode() {
  switch(mode) {
    case 1: // full step
      gpio_put(X_Mode0, 0);
      gpio_put(X_Mode1, 0);
      gpio_put(X_Mode2, 0);
      gpio_put(Y_Mode0, 0);
      gpio_put(Y_Mode1, 0);
      gpio_put(Y_Mode2, 0);
      printf("Mode set: %d\n", mode);
      break;
    case 2: // half step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 0);
      gpio_put(X_Mode2, 0);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 0);
      gpio_put(Y_Mode2, 0);
      printf("Mode set: %d\n", mode);
      break;
    case 4: // quarter step
      gpio_put(X_Mode0, 0);
      gpio_put(X_Mode1, 1);
      gpio_put(X_Mode2, 0);
      gpio_put(Y_Mode0, 0);
      gpio_put(Y_Mode1, 1);
      gpio_put(Y_Mode2, 0);
      printf("Mode set: %d\n", mode);
      break;
    case 8: // eighth step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 1);
      gpio_put(X_Mode2, 0);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 1);
      gpio_put(Y_Mode2, 0);
      printf("Mode set: %d\n", mode);
      break;
    case 16: // sixteenth step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 1);
      gpio_put(X_Mode2, 1);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 1);
      gpio_put(Y_Mode2, 1);
      printf("Mode set: %d\n", mode);
      break;
    case 32: // thirty-second step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 0);
      gpio_put(X_Mode2, 1);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 0);
      gpio_put(Y_Mode2, 1);
      printf("Mode set: %d\n", mode);
      break;    
      default:
      printf("Invalid microstepping mode");
  }
}

// Functions for g-code

// convert mm to steps
int mm_to_steps(float mm) {
  int steps = (int)(fabs(mm) * steps_per_mm); // fabs() = absolute value for floats
  if (steps == 0 && mm != 0) steps = 1;
  return steps;
}

// pulse functions for manual mode (uses step_delay_us instead of high/low delay for consistent speed)
void pulse_x_gcode() {
  gpio_put(X_STEP, 1);
  sleep_us(step_delay_us);
  gpio_put(X_STEP, 0);
  sleep_us(step_delay_us);
}

void pulse_y_gcode() {
  gpio_put(Y_STEP, 1);
  sleep_us(step_delay_us);
  gpio_put(Y_STEP, 0);
  sleep_us(step_delay_us);
}

void pulse_z_gcode() {
  gpio_put(Z_STEP, 1);
  sleep_us(step_delay_us);
  gpio_put(Z_STEP, 0);
  sleep_us(step_delay_us);
}

// Move each axis
void move_x(float mm) {
  int steps = mm_to_steps(mm);
  if (steps == 0) return;
  gpio_put(X_DIR, mm > 0);
  for (int i = 0; i < steps; i++) {
    pulse_x_gcode();
  }
  pos_x += mm;
}

void move_y(float mm) {
  int steps = mm_to_steps(mm);
  if (steps == 0) return;
  gpio_put(Y_DIR, mm > 0);
  for (int i = 0; i < steps; i++) {
    pulse_y_gcode();
  }
  pos_y += mm;
}

void move_z(float mm) {
  int steps = mm_to_steps(mm);
  if (steps == 0) return;
  gpio_put(Z_DIR, mm > 0);
  for (int i = 0; i < steps; i++) {
    pulse_z_gcode();
  }
  pos_z += mm;
}

// function to process user inputs into the buffer array
void process_input() {

  // to handle manual mode
  if (manual_mode) {
    int c = getchar_timeout_us(0);
   
    if (c != PICO_ERROR_TIMEOUT) {
      switch(c) {
        case '1': step_size = 0.075; printf("Step size set to 0.075mm: IN PRECISE MODE\n"); break;
        case '2': step_size = 0.3; printf("Step size set to 0.3mm: IN NORMAL MODE\n"); break;
        case '3': step_size = 0.6; printf("Step size set to 0.6mm: IN FAST MODE\n"); break;
        case 'w': case 'W': key_w = true; break;
        case 's': case 'S': key_s = true; break;
        case 'a': case 'A': key_a = true; break;
        case 'd': case 'D': key_d = true; break;
        case 'o': case 'O': key_o = true; break;
        case 'p': case 'P': key_p = true; break;
        case 'l': case 'L': printf("Current position - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z); break;
        case 'h': case 'H':
        origin_x = pos_x; origin_y = pos_y; origin_z = pos_z; origin_set = true;
        printf("Origin set to current position - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", origin_x, origin_y, origin_z);
        break;
        case 'r': case 'R':
        if (origin_set) {
          move_x(origin_x - pos_x);
          move_y(origin_y - pos_y);
          move_z(origin_z - pos_z);
          printf("Returned to origin - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z);
        } else {
          printf("Origin not set. Use 'setorigin' command to set the origin point first.\n");
        }
        break;
        case 'm': case 'M': manual_mode = false;
                  printf("Exiting manual mode. Returning to default mode.\n");
                  break;
                  default: break;
      }
    } else {
      // release all keys when no input
      key_w = key_a = key_s = key_d = key_o = key_p = false;
    }
    /// execute movement based on key states
  if (key_w) move_y(step_size);
  if (key_s) move_y(-step_size);
  if (key_a) move_x(-step_size);
  if (key_d) move_x(step_size);
  if (key_o) move_z(step_size);
  if (key_p) move_z(-step_size);
 return;
}
 
 


  if (command_complete) {
    return; // if command is already complete, ignore further input until processed
  }

  int c = getchar_timeout_us(0);
 
  if (c != PICO_ERROR_TIMEOUT) {
    // process the input character

    if (c == '\r' || c == '\n') {

    command_buffer[buffer_index] = '\0'; // creates a C string
    command_complete = true; // sets flag to indicate command is complete
    return;

  }
  // code to handle backspace input
    if (c == '\b' && buffer_index > 0) {

      buffer_index--; // moves index back to remove last character
      command_buffer[buffer_index] = '\0'; // null-terminate the string after backspace
      return;

    }

    // adds input into buffer
    if (buffer_index < buffer_size - 1) {

      command_buffer[buffer_index++] = c; // adds character to buffer and increments index
      if (c >= 32 && c <= 126) {
      printf("%c", c); // echoes the character back to the user
      }

     } else {

      printf("Error: Command buffer overflow. Maximum command length is %d characters.\n", buffer_size - 1);
      buffer_index = 0; // reset buffer index to prevent overflow
      command_buffer[0] = '\0'; // clear the
     
     }
  }
}

// function to process and execute the command from the buffer
void process_command() {

  // remove any non printable trash characters from the command buffer
  for (int i = 0; i < buffer_index; i++) {
    if (command_buffer[i] < 32 || command_buffer[i] > 126) {
      command_buffer[i] = '\0'; // replace non-printable characters with null terminator
    }
  }

  // arrays to store different types of commands
  char command[10];
  char value_str[10];
  char integer[10] = {0}; // array to store integer value as string for parsing

  // uses sscanf to parse the commend and value from the buffer
  int count = sscanf(command_buffer, "%s %s %s", command, value_str, integer);
  printf("Command: %s, Value: %s, integer: %s\n", command, value_str, integer); // prints the parsed commend and value for debugging

  // checks if the commend is valid and executes the corresponding action
  if (count >= 1) {

    if (strcmp(command, "delay") == 0 && count == 3) {

      int delay_value = 0; // variable to store the delay value
       sscanf(integer, "%d", &delay_value); // converts the integer value from string to integer

      // sets the delay for pulse depending on the command

      if (strcmp(value_str, "high") == 0) {

        high_delay_us = delay_value; // sets the high delay to the value from the commend
        printf("High delay set to: %d microseconds\n", high_delay_us);

      } else if (strcmp(value_str, "low") == 0) {

        low_delay_us = delay_value; // sets the low delay to the value from the commend
        printf("Low delay set to: %d microseconds\n", low_delay_us);

      } else {

        printf("Invalid delay type. Use 'high' or 'low'.\n");
      }

    } else if (strcmp(command, "axis") == 0 && count == 2) {

      axis_selection = value_str[0]; // sets the axis selection to the value from the commend
      printf("Axis selected: %c\n", axis_selection);

    } else if (strcmp(command, "mode") == 0 && count == 2) {

      sscanf(value_str, "%d", &mode); // converts the value from string to integer
      set_microstepping_mode(); // function call to set the microstepping mode based on the value from the commend
      printf("Microstepping mode set to: %d\n", mode);

    } else if (strcmp(command, "fwd") == 0 && count == 2) {

      forward = true; // sets the direction to forward
      set_stepper_direction(); // sets the direction to forward
      printf("Direction set to forward\n");
      sscanf(value_str, "%d", &steps); // converts the value from string to integer
      execute_n_steps(); // function call to execute the number of steps from the commend
      printf("Executed %d steps\n", steps);

    } else if (strcmp(command, "rev") == 0 && count == 2) {

      forward = false; // sets the direction to reverse
      set_stepper_direction(); // sets the direction to reverse
      printf("Direction set to reverse\n");
      sscanf(value_str, "%d", &steps); // converts the value from string to integer
      execute_n_steps(); // function call to execute the number of steps from the commend
      printf("Executed %d steps\n", steps);

    } else if (strcmp(command, "manual") == 0 && count ==1) {
      manual_mode = true; // sets manual mode to true
      memset(command_buffer, 0, buffer_size);
      buffer_index = 0;
      command_complete = false;
      printf("Entering manual mode\n");
      printf("Use W/A/S/D for Y+/X-/Y-/X+ movement and O/P for Z+/Z- movement.\n");
      printf("Press L to display current position\n");
      printf("Press H to set current position as origin and R to return to origin.\n");
      printf("Press m to exit manual mode and return to default mode.\n");

    } else if (strcmp(command, "LC") == 0 && count == 1) {
      printf("Current position - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z);

    } else if (strcmp(command, "setorigin") == 0 && count == 1) {
      //set current position as origin
      origin_x = pos_x;
      origin_y = pos_y;
      origin_z = pos_z;
      origin_set = true;
      printf("Origin set to current position - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", origin_x, origin_y, origin_z);

    } else if (strcmp(command, "reset") == 0 && count == 1) {
      // move back to origin if set
      if (!origin_set) {
        printf("Origin not set. Use 'setorigin' command to set the origin point first.\n");
      } else {
        float move_x_mm = origin_x - pos_x;
        float move_y_mm = origin_y - pos_y;
        float move_z_mm = origin_z - pos_z;
        move_x(move_x_mm);
        move_y(move_y_mm);
        move_z(move_z_mm);
        printf("Returned to origin - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z);
      }
    }else if (strcmp(command, "help") == 0) {  

      printf("Available commands:\n");
      printf("delay high <value> - Set the high delay in microseconds\n");
      printf("delay low <value> - Set the low delay in microseconds\n");
      printf("axis <x/y/z> - Select the axis to control\n");
      printf("mode <1/2/4/8/16/32> - Set the microstepping mode\n");
      printf("fwd <steps> - Move forward a specified number of steps\n");
      printf("rev <steps> - Move reverse a specified number of steps\n");
      printf("help - Show this help message\n");

    } else {

      printf("Invalid command or missing value\n");
      return;

    }
  } else {

    printf("Invalid command format\n");
    return;

  }
}

int main(void) {

  stdio_init_all();

  sleep_ms(2000); // delay to allow time for the serial monitor to connect

  init_stepper_pins(); // initalizes pins inside main() with function call

  set_microstepping_mode(); // sets the default microstepping mode

  while (true) {

    // function call to process user input

    process_input();

    // checks if command is complete
    if (command_complete) {

      process_command(); // function call to process the command

      // reset buffer and index for next command
      memset(command_buffer, 0, buffer_size); // clear the command buffer
      buffer_index = 0;
      command_complete = false;

    }
  }
  return 0;
}
