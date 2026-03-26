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

// function to process user inputs into the buffer array
void process_input() {

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
      printf("%c", c); // echoes the character back to the user
      printf("[%d]", c); // prints the ASCII value of the character for debugging

     } else {

      printf("Error: Command buffer overflow. Maximum command length is %d characters.\n", buffer_size - 1);
      buffer_index = 0; // reset buffer index to prevent overflow
      command_buffer[0] = '\0'; // clear the
     
     }
  }
}

// function to process and execute the command from the buffer
void process_commend() {

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

    } else if (strcmp(command, "help") == 0) {  

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

      process_commend(); // function call to process the commend

      // reset buffer and index for next command
      memset(command_buffer, 0, buffer_size); // clear the command buffer
      buffer_index = 0;
      command_complete = false;

    }
  }
  return 0;
}

