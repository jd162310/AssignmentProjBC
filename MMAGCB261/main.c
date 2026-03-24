/**************************************************************
 * main.c
 * rev 1.0 20-Mar-2026 bjays
 * MMAGCB261
 * ***********************************************************/


#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>

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

int high_delay_us = 1000; // microseconds
int low_delay_us = 1000; // microseconds

// axis selection variable
char axis_selection = 'x'; // default to x-axis

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

// Function to send pulse signal to stepper motor
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
void execute_n_steps(int steps) {
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
void set_stepper_direction(bool forward) {
  // sets direction forward or backward based on user input
  gpio_put(X_DIR, forward); 
  gpio_put(Y_DIR, forward);
  gpio_put(Z_DIR, forward);
}

// function to set up microstepping mode
void set_microstepping_mode(int mode) {
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
int main(void) {

  stdio_init_all(); 

  init_stepper_pins(); // initalizes pins inside main() with function call

  while (true) {
    // obtains user input
    int c = getchar_timeout_us(0);
    if (c != PICO_ERROR_TIMEOUT) {
      switch(c) {
          case 'f':
          case 'F':
          execute_n_steps(1);
          break;
          case 'g':
          case 'G':
          execute_n_steps(10);
          break;
          case 'h':
          case 'H':
          execute_n_steps(100);
          break;
          case 'j':
          case 'J':
          execute_n_steps(200);
          break;
          case 'd':
          case 'D':
          set_stepper_direction(true);
          break;
          case 'a':
          case 'A':
          set_stepper_direction(false);
          break;
          case '0':
          set_microstepping_mode(1);
          break;
          case '1':
          set_microstepping_mode(2);
          break;
          case '2':
          set_microstepping_mode(4);
          break;
          case '3':
          set_microstepping_mode(8);
          break;
          case '4':
          set_microstepping_mode(16);
          break;
          case '5':
          set_microstepping_mode(32);
          break;
          case 'x':
          case 'X':
          case 'y':
          case 'Y':
          case 'z':
          case 'Z':
          axis_selection = c;
          printf("Axis selected: %c\n", axis_selection);
          break;
          default:
          printf("Invalid input");
      }
  }
}
  return 0;
}

