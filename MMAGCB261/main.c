
#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "math.h"

// global variables
// Define axis pins
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
#define buffer_size 100
char command_buffer[buffer_size]; // array to store characters from user input
int buffer_index = 0; // index to keep track of position in buffer
bool command_complete = false; // flag to indicate when a command is complete

// global variable for direction
bool forward = true; // defaults to forward direction

// global variable for mircostepping mode
int mode = 1; // defaults to full step mode

// glodal variable for number of steps
int steps = 0; // defaults to 0 steps

// variable for spindle motor control
#define SPINDLE_PWM_PIN 17
uint slice_num; // variable to store the PWM slice number for spindle control
uint16_t spindle_speed = 0; // variable to store the spindle speed as a PWM

// manual mode variables
float pos_x = 0, pos_y = 0, pos_z = 0; // axis postion variables
float steps_per_mm = 40; // number of steps per mm of movement
float step_size = 0.025; // mm for each step
float mm = 0; // variable to store mm value 

// flags for different modes
bool manual_mode = false; // flag to set manual mode on or off
bool default_mode = true; // flag for default mode

// key state tracking
bool key_w = false; // Y+
bool key_s = false; // Y-
bool key_a = false; // X-
bool key_d = false; // X+
bool key_q = false; // Z+
bool key_e = false; // Z-
bool key_o = false; // S+
bool key_p = false; // S-
bool key_l = false; // display position
bool key_h = false; // set origin
bool key_r = false; // return to origin

// origin variable initialization 
float x_origin = 0, y_origin = 0, z_origin = 0;
bool origin_set = false; // flag for setting origin

// Function for pin initialization
void init_stepper_pins() {

  // set the pins to output
  // set enable pin
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

void init_spindle_motor() {

  gpio_set_function(SPINDLE_PWM_PIN, GPIO_FUNC_PWM); // sets spindle pin to pwm 

  slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN); // finds the slice number for the pwm pin
  
  pwm_clear_irq(slice_num); // clears the interrupt flag for the slice 

  pwm_config config = pwm_get_default_config(); // gets defualt PWM configuration

  pwm_config_set_clkdiv(&config, 125.0);
  pwm_config_set_wrap(&config, 65535);

  pwm_init(slice_num, &config, true); // applies config and starts PWM

  pwm_set_gpio_level(SPINDLE_PWM_PIN, 0);
  printf("PWM set to %d\n", SPINDLE_PWM_PIN);

}

// Functions to send pulse signal to each axis stepper motor
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

// Function for spindle motor control
void spindle_control() {

  pwm_set_gpio_level(SPINDLE_PWM_PIN, spindle_speed);

}

// Function to execute a number of steps
void execute_n_steps() {

  // calculates mm moved
  float mm_moved = steps / steps_per_mm;

  if (!forward) {
    mm_moved = -mm_moved; // turns the mm_moved into negative if going backwards 
  }

  for (int i = 0; i < steps; i++) {

    switch (axis_selection) {
      case 'x': case 'X':
        send_pulse_to_stepperx();
        break;
      case 'y': case 'Y':
        send_pulse_to_steppery();
        break;
      case 'z': case 'Z':
        send_pulse_to_stepperz();
        break;
    }
  }

  // updates the position of the axis after movement is complete
  switch (axis_selection) {
    case 'x': case 'X':
      pos_x += mm_moved; // updates the x axis position
      break;
    case 'y': case 'Y':
      pos_y += mm_moved; // updates the y axis position
      break;
    case 'z': case 'Z':
      pos_z += mm_moved; // updates the z axis position
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

// converts mm to steps
void mm_to_steps() {

  steps = (int)roundf(fabs(mm) * steps_per_mm); // converts mm distance into motor steps

  // sets the steps to 1 if the mm movement is small and gets rounded down to 0
  if (steps == 0 && mm != 0) {
    steps = 1;
  }
  return;
}
  
// function to execute movement based on key states
void execute_manual_movement() {

  mm = step_size; // sets the mm variable to the step size for each movement
  if (!forward) {
    mm = -mm; // turns the mm movement negative if going backwards
  }

  bool move = false; // flag to check if any movement is needed
  if (key_a || key_d || key_w || key_s || key_q || key_e) {
    move = true; // sets flag to true if any keys are pressed
  }

  int speed = 0; // variable to store the spindle speed

  // checks which keys are pressed and moves the corresponding axis
  if (key_w) { // y+
    axis_selection = 'y';
    forward = true;
  } else if (key_s) { // y-
    axis_selection = 'y';
    forward = false;
  } else if (key_d) { // x+
    axis_selection = 'x';
    forward = true;
  } else if (key_a) { // x-
    axis_selection = 'x';
    forward = false;
  } else if (key_e) { // z+
    axis_selection = 'z';
    forward = true;
  } else if (key_q) { // z-
    axis_selection = 'z';
    forward = false;
  } else if (key_p) { // s+

    speed += 25; // increases spindle speed by 25 percent
    if (speed > 100) 
    speed = 100; // caps the speed at 100 percent
    spindle_speed = (speed * 65535) / 100; // sets spindle speed using percentage
    spindle_control(); // function call to update spindle speed 

  } else if (key_o) { // s-

    speed -= 25; // decreases spindle speed by 25 percent
    if (speed < 0) 
    speed = 0; // caps the speed from going negative
    spindle_speed = (speed * 65535) / 100; // sets spindle speed using percentage
    spindle_control(); // function call to update spindle speed 

  } else if (key_l) { // display current position
    printf("Current position - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z);
  } else if (key_h) { // sets origin
    x_origin = pos_x;
    y_origin = pos_y;
    z_origin = pos_z;
    origin_set = true;
    printf("Origin set to current position - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", x_origin, y_origin, z_origin);
  } else if (key_r) { // returns to origin

    if (origin_set) {
      
      // calculates the distance to move back to origin
      float delta_x = x_origin - pos_x;
      float delta_y = y_origin -pos_y;
      float delta_z = z_origin - pos_z;

      // moves back to origin
      if (delta_x != 0 || delta_y != 0 || delta_z != 0) {

        axis_selection = 'x';
        forward = (delta_x > 0) ? true : false; // sets direction based on whether delta_x is positive or negative
        mm = fabs(delta_x); // sets mm to the aboslute value of delta_x for movement
        mm_to_steps(); // converts mm movement into steps
        execute_n_steps(); // executes the steps to move the motor

        axis_selection = 'y';
        forward = (delta_y > 0) ? true : false; // sets direction based on whether delta_y is positive or negative
        mm = fabs(delta_y); // sets mm to the aboslute value of delta_y for movement
        mm_to_steps(); // converts mm movement into steps
        execute_n_steps(); // executes the steps to move the motor

        axis_selection = 'z';
        forward = (delta_z > 0) ? true : false; // sets direction based on whether delta_z is positive or negative
        mm = fabs(delta_z); // sets mm to the aboslute value of delta_z for movement
        mm_to_steps(); // converts mm movement into steps
        execute_n_steps(); // executes the steps to move the motor

        // prints the current position after returning to origin
        printf("Returned to origin - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z);
      } else {
        // if already at origin, just print the current position 
        printf("Already at origin - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z);
      }
    } else {
      printf("Origin not set. Press the H key to set a origin point.\n");
    }
    }

    if (move) {
    set_stepper_direction(); // sets the direction with a function call
    mm_to_steps(); // converts the mm movemnet into steps
    execute_n_steps(); // executes the steps to move the motor
    } 

    // resets key states after movement is executed to stop continuous movement
    key_w = key_s = key_a = key_d = key_q = key_e = key_o = key_p = key_l = key_h = key_r = false;

    sleep_ms(100); // small delay to prevent multiple inputs from being processed too quickly


}

// function to process user inputs into the buffer array
void process_input() {

  if (command_complete && default_mode) {
    return; // if command is already complete, ignore further input until processed
  }

  int c = getchar_timeout_us(0);
  
  if (c != PICO_ERROR_TIMEOUT) {

    // key state tracking for manual mode
    if (manual_mode) {
    switch(c) {
        case 'w': case 'W': 
        key_w = true;
        break;
        case 's': case 'S':
        key_s = true;
        break;
        case 'a': case 'A':
        key_a = true;
        break;
        case 'd': case 'D':
        key_d = true; 
        break;
        case 'o': case 'O':
        key_o = true;
        break;
        case 'p': case 'P':
        key_p = true;
        break;
        case 'q': case 'Q':
        key_q = true;
        break;
        case 'e': case 'E':
        key_e = true;
        break;
        case 'l': case 'L':
        key_l = true;
        break;
        case 'h': case 'H':
        key_h = true;
        break;
        case 'r': case 'R':
        key_r = true;
        break;
        case '1': 
        step_size = 0.025;
        mode = 1;
        set_microstepping_mode();
        printf("Step size set to 0.025mm: IN NORMAL MODE\n");
        break;
        case '2':
        step_size = 0.0125;
        mode = 2;
        set_microstepping_mode();
        printf("Step size set to 0.0124mm: HALF STEP MODE\n");
        break;
        case '3':
        step_size = 0.00625;
        mode = 4; 
        set_microstepping_mode();
        printf("Step size set to 0.00625mm: QUARTER STEP MODE\n");
        break;
        case '4':
        step_size = 0.003125;
        mode = 8;
        set_microstepping_mode();
        printf("Step size set to 0.003125mm: EIGHTH STEP MODE\n");
        break;
        case '5':
        step_size = 0.0015625;
        mode = 16;
        set_microstepping_mode();
        printf("Step size set to 0.0015625mm: SIXTEENTH STEP MODE\n");
        break;
        case '6':
        step_size = 0.00078125;
        mode = 32;
        set_microstepping_mode();
        printf("Step size set to 0.00078125mm: THIRTY-SECOND STEP MODE\n");
        break;
        case 'm': case 'M':
        manual_mode = false;
        default_mode = true;
        printf("Exiting manual mode. Returning to default mode.\n");
        break;
        default: 
        break;
    }
  }

  if (default_mode) {

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

     } else {

      printf("Error: Command buffer overflow. Maximum command length is %d characters.\n", buffer_size - 1);
      buffer_index = 0; // reset buffer index to prevent overflow
      command_buffer[0] = '\0'; // clear the
     }
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

  // checks if the commend is valid and executes the corresponding command
  if (count >= 1 && default_mode == true) {

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

    } else if (strcmp(command, "spin") == 0 && count == 2) { 

      printf("RAW INPUT: %s\n", command_buffer);
      printf("Parsed speed string: %s\n", value_str);

      int speed = 0; // init variable speed
      sscanf(value_str, "%d", &speed); // saves user input into speed
      printf("speed is %d\n", speed);
      // error handling for speed inputs
      if (speed < 0 || speed > 100) {

        printf("inavlid input\n");
        return;

      }

      // sets the spindle speed to whatever percent the user inputs
      spindle_speed = (65535 * speed) / 100;
      printf("spindle speed set to: %d\n", spindle_speed); // debug helper
      spindle_control(); 
      printf("spindle speed at %d percent\n", speed);

    } else if (strcmp(command, "help") == 0) {  

      printf("Available commands:\n");
      printf("delay high <value> - Set the high delay in microseconds\n");
      printf("delay low <value> - Set the low delay in microseconds\n");
      printf("axis <x/y/z> - Select the axis to control\n");
      printf("mode <1/2/4/8/16/32> - Set the microstepping mode\n");
      printf("fwd <steps> - Move forward a specified number of steps\n");
      printf("rev <steps> - Move reverse a specified number of steps\n");
      printf("spin <value> - set the spindle speed\n");
      printf("all values must be between 0-1000 except for spin which is 0-50\n");
      printf("enter manual to switch to manual mode\n");
      printf("help - Show this help message\n");

    } else if (strcmp(command, "manual") == 0 && count == 1) {

      manual_mode = true; // sets manual mode to true
      memset(command_buffer, 0, buffer_size); // clear the command buffer
      buffer_index = 0; // reset the buffer index
      default_mode = false; // reset the command complete flag
      printf("Entering manual mode\n");
      printf("Use W/A/S/D/E/Q for Y, X, and Z axis movement and O/P for spindle speed control.\n");
      printf("Press L to display current position\n");
      printf("Press H to set current position as origin and R to return to origin.\n");
      printf("Press m to exit manual mode and return to default mode.\n");
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

  sleep_ms(2000); // delay to allow time for serial to connect

  // function calls to initalize pins 
  init_stepper_pins(); 
  init_spindle_motor();

  printf("enter help for a list of commands\n"); 

  while (true) {

    // function call to process user input
    process_input();

    if (manual_mode) {

      // function call to execute movement based on key states
    execute_manual_movement();

  } else {

    // checks if command is complete
    if (command_complete) {

      process_commend(); // function call to process the commend

      // reset buffer and index for next command
      memset(command_buffer, 0, buffer_size); // clear the command buffer
      buffer_index = 0;
      command_complete = false;

    }
  }
} 
  return 0;
}