
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
int delay = 1000; // mircoseconds

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
int speed = 0; // variable to store the spindle speed

// manual mode variables
float pos_x = 0, pos_y = 0, pos_z = 0; // axis postion variables
float steps_per_mm = 40; // number of steps per mm of movement
float step_size = 0.025; // mm for each step
float mm = 0; // variable to store mm value 

// flags for different modes
bool manual_mode = false; // flag to set manual mode on or off
bool default_mode = true; // flag for default mode
bool auto_mode = false; // flag for auto mode

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
bool key_u = false; // unset origin


// origin variable initialization 
float x_origin = 0, y_origin = 0, z_origin = 0;
bool origin_set = false; // flag for setting origin

// G-code variables
bool absolute_pos = true; // flag for absolute vs relative positioning mode 

//function for pin initialization
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

}

// Functions to send pulse signal to each axis stepper motor
void send_pulse_to_stepperx() {

  gpio_put(X_STEP, 1);
  sleep_us(delay);
  gpio_put(X_STEP, 0);
  sleep_us(delay);

}

void send_pulse_to_steppery() {

  gpio_put(Y_STEP, 1);
  sleep_us(delay);
  gpio_put(Y_STEP, 0);
  sleep_us(delay);

}

void send_pulse_to_stepperz() {
  gpio_put(Z_STEP, 1);
  sleep_us(delay);
  gpio_put(Z_STEP, 0);
  sleep_us(delay);

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
      break;
    case 2: // half step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 0);
      gpio_put(X_Mode2, 0);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 0);
      gpio_put(Y_Mode2, 0);
      break;
    case 4: // quarter step
      gpio_put(X_Mode0, 0);
      gpio_put(X_Mode1, 1);
      gpio_put(X_Mode2, 0);
      gpio_put(Y_Mode0, 0);
      gpio_put(Y_Mode1, 1);
      gpio_put(Y_Mode2, 0);
      break;
    case 8: // eighth step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 1);
      gpio_put(X_Mode2, 0);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 1);
      gpio_put(Y_Mode2, 0);
      break;
    case 16: // sixteenth step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 1);
      gpio_put(X_Mode2, 1);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 1);
      gpio_put(Y_Mode2, 1);
      break;
    case 32: // thirty-second step
      gpio_put(X_Mode0, 1);
      gpio_put(X_Mode1, 0);
      gpio_put(X_Mode2, 1);
      gpio_put(Y_Mode0, 1);
      gpio_put(Y_Mode1, 0);
      gpio_put(Y_Mode2, 1);
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
  
// function to move x and y axis simultaneously
void move_xy(float mm_x, float mm_y) {

  // calculates the number of steps needed
    int steps_x = (int)(fabs(mm_x) * steps_per_mm);
    int steps_y = (int)(fabs(mm_y) * steps_per_mm);
    if (steps_x == 0 && steps_y == 0) return; // if no movement is needed, return

    // set directions for x and y axis
    gpio_put(X_DIR, mm_x > 0);
    gpio_put(Y_DIR, mm_y > 0);

    int max_steps = steps_x > steps_y ? steps_x : steps_y; // finds the maximum number of steps needed for either axis

    for (int i = 0; i < max_steps; i++) {
      if (i < steps_x) {
        gpio_put(X_STEP, 1);
        sleep_ms(500);
        gpio_put(X_STEP, 0);
      }
      if (i < steps_y) {
        gpio_put(Y_STEP, 1);
        sleep_ms(500);
        gpio_put(Y_STEP, 0);
      }
      sleep_ms(500);
    }
    pos_x += mm_x; // updates the x axis position
    pos_y += mm_y; // updates the y axis position
}

// function to draw a arc based on G-code command
void draw_arc(float end_x, float end_y, float i, float j, int clockwise) {

  // calculates the center of the arc
  float centre_x = pos_x + i;
  float centre_y = pos_y + j;
  float radius = sqrt(i*i + j*j);

  // calculates the start and end angles in radians
  float start_angle = atan2(pos_y - centre_y, pos_x - centre_x);
  float end_angle = atan2(end_y - centre_y, end_x - centre_x);
  
  // number of steps for smooth arc
  int steps = (int)(roundf(radius * fabs(end_angle - start_angle) * steps_per_mm)); // calculates steps based on arc length

  // variables to keep track of current angle and position
  float current_angle, angle_step, dx, dy;
  
  // executes the arc movement by incrementing the angle and calculating the corresponding x and y steps
  if (clockwise) {

    if (start_angle < end_angle) {
      start_angle += 2 * M_PI; // adjusts the start angle to start from the correct position for clockwise movement
    }
    angle_step = (start_angle - end_angle) / steps; // calculates the angle to increment for each step
    current_angle = start_angle; // initializes the current angle to the start angle

    // loops through the number of steps
    for (int step = 0; step < steps; step++) {
      current_angle -= angle_step; // decrements the angle for clockwise movement 
      float x = centre_x + radius * cos(current_angle); // calculates the x position for current angle
      float y = centre_y + radius * sin(current_angle); // calculates the y position for current angle

      // calculates the mm to move for the step
      dx = x - pos_x;
      dy = y - pos_y; 

      if (dx != 0) {
        axis_selection = 'x';
        forward = (dx > 0);
        mm = fabs(dx);
        mm_to_steps();
        set_stepper_direction();
        execute_n_steps();
      }
      if (dy != 0) {
        axis_selection = 'y';
        forward = (dy > 0);
        mm = fabs(dy);
        mm_to_steps();
        set_stepper_direction();
        execute_n_steps();
       }
    }
  } else { // if anit-clockwise
    if (start_angle >= end_angle) {
      end_angle = end_angle + 2 * M_PI;
    }
    angle_step = (end_angle - start_angle) / steps;
    current_angle = start_angle;
    for (int step = 0; step < steps; step++) {
      current_angle += angle_step;
      float x = centre_x + radius * cos(current_angle);
      float y = centre_y + radius * sin(current_angle);

      //calculate the mm to move for this step
      dx = x - pos_x;
      dy = y - pos_y;

      if (dx != 0) {
        axis_selection = 'x';
        forward = (dx > 0);
        mm = fabs(dx);
        mm_to_steps();
        set_stepper_direction();
        execute_n_steps();
      }
      if (dy != 0) {
        axis_selection = 'y';
        forward = (dy > 0);
        mm = fabs(dy);
        mm_to_steps();
        set_stepper_direction();
        execute_n_steps();
       }
      }
  }

  // final exact movement to end point
  dx = end_x - pos_x;
  dy = end_y - pos_y; 
  
  if (dx != 0) {
        axis_selection = 'x';
        forward = (dx > 0);
        mm = fabs(dx);
        mm_to_steps();
        set_stepper_direction();
        execute_n_steps();
      }
      if (dy != 0) {
        axis_selection = 'y';
        forward = (dy > 0);
        mm = fabs(dy);
        mm_to_steps();
        set_stepper_direction();
        execute_n_steps();
       }
  // final exact move to end point
  dx = end_x - pos_x;
  dy = end_y - pos_y;

  if (dx != 0) {
    axis_selection = 'x';
    forward = (dx > 0);
    mm = fabs(dx);
    mm_to_steps();
    set_stepper_direction();
    execute_n_steps();
  }
  if (dy != 0) {
    axis_selection = 'y';
    forward = (dy > 0);
    mm = fabs(dy);
    mm_to_steps();
    set_stepper_direction();
    execute_n_steps();
  }
  pos_x = end_x;
  pos_y = end_y;
}

// G-code interpretor function
void process_gcode_command(char *gcode_line) {

  char cmd[10];
  char *ptr = gcode_line;
  int i = 0;
  float x = 0, y = 0, z = 0, f =0, s = 0;

  // parses the command from the gcode line
  while (*ptr && *ptr != ' ' && i < 9) {
    cmd[i++] = *ptr++;
  }

  cmd[i] = '\0'; // null-terminate the command string
  ptr = gcode_line; // resets pointer to the start of the line

  // parses the command parameters
  while (*ptr) {
    if (*ptr == 'x' || *ptr == 'X') {
      x = atof(ptr + 1); // converts the x value from string to float
    }
    if (*ptr == 'y' || *ptr == 'Y') {
      y = atof(ptr + 1); // converts the y value from string to float
    }
    if (*ptr == 'z' || *ptr == 'Z') {
      z = atof(ptr + 1); // converts the z value from string to float
    }
    if (*ptr == 'f' || *ptr == 'F') {
      f = atof(ptr + 1); // converts the freedrate value from string to float
    }
    if (*ptr == 's' || *ptr == 'S') {
      s = atof(ptr + 1); // converts the spindle speed value from string to float
    }
    ptr++; // moves pointer to the next character
  }

  // processes the G-code command and executes
  // checks whether the feedrate is valid
  if (f > 0) {
    delay = (int)(200000 / f); // calculates the delay based on the feedrate
    if (delay < 100) delay = 100; // sets the minimum delay to prevent going too fast
    if (delay > 2000) delay = 2000; // sets the maximum delay to prevent going too slow
  }
  if (strcmp(cmd, "G0") == 0 || strcmp(cmd, "G00") == 0) { // rapid move control
    if (x != 0) {
      axis_selection = 'x';
      forward = (x > 0); // sets direction based on whether x is positive or negative
      mm = fabs(x); // sets mm to the aboslute value of x for movement
      mm_to_steps(); // converts mm movement into steps
      set_stepper_direction(); // sets the direction 
      execute_n_steps(); // executes the steps
    }
    if (y != 0) {
      axis_selection = 'y';
      forward = (y > 0); // sets direction based on whether y is positive or negative
      mm = fabs(y); // sets mm to the aboslute value of y for movement
      mm_to_steps(); // converts mm movement into steps
      set_stepper_direction(); // sets the direction 
      execute_n_steps(); // executes the steps
    }
    if (z != 0) {
      axis_selection = 'z';
      forward = (z > 0); // sets direction based on whether z is positive or negative
      mm = fabs(z); // sets mm to the aboslute value of z for movement
      mm_to_steps(); // converts mm movement into steps
      set_stepper_direction(); // sets the direction 
      execute_n_steps(); // executes the steps
    } 
  } else if (strcmp(cmd, "G01") == 0 || strcmp(cmd, "G1") == 0) { // linear move control

    if (x != 0 || y != 0) {
      move_xy(x, y); // function call to move in the xy plane simultaneously
    }
    if (z != 0) {
      axis_selection = 'z';
      forward = (z > 0); // sets direction based on whether z is positive or negative
      mm = fabs(z); // sets mm to the aboslute value of z for movement
      mm_to_steps(); // converts mm movement into steps
      set_stepper_direction(); // sets the direction
      execute_n_steps(); // executes the steps
    }
  } else if (strcmp(cmd, "M03") == 0 || strcmp(cmd, "M3") == 0) { // spindle on control

      int speed = (int)((s * 65535) / 100); // converts the spindle speed from percentage to PWM value
      if (speed < 0) speed = 0; // caps the speed from going negative
      if (speed > 65535) speed = 65535; // caps the speed at maximum PWM value
      spindle_speed = speed; // sets the spindle speed variable
      spindle_control(); // function call to update spindle speed
  }
  else if (strcmp(cmd, "M05") == 0 || strcmp(cmd, "M5") == 0) { // spindle off control

      spindle_speed = 0; // sets spindle speed to 0 (turns off)
      spindle_control(); // function call to update spindle speed
  }
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

  // checks which keys are pressed and moves the corresponding axis
  if (key_w) { // y+
    axis_selection = 'y';
    forward = true;
    printf("Y+\n");
  } else if (key_s) { // y-
    axis_selection = 'y';
    forward = false;
    printf("Y-\n");
  } else if (key_d) { // x+
    axis_selection = 'x';
    forward = true;
    printf("X+\n");
  } else if (key_a) { // x-
    axis_selection = 'x';
    forward = false;
    printf("X-\n");
  } else if (key_e) { // z+
    axis_selection = 'z';
    forward = true;
    printf("Z+\n");
  } else if (key_q) { // z-
    axis_selection = 'z';
    forward = false;
    printf("Z-\n");
  } else if (key_p) { // s+

    speed += 25; // increases spindle speed by 25 percent
    if (speed > 100) {
    speed = 100; // caps the speed at 100 percent
    }
    spindle_speed = (speed * 65535) / 100; // sets spindle speed using percentage
    spindle_control(); // function call to update spindle speed 

  } else if (key_o) { // s-

    speed -= 25; // decreases spindle speed by 25 percent
    if (speed < 0) {
    speed = 0; // caps the speed from going negative
    }
    spindle_speed = (speed * 65535) / 100; // sets spindle speed using percentage
    spindle_control(); // function call to update spindle speed 

  } else if (key_l) { // display current position
    printf("Current position - X: %.2f mm, Y: %.2f mm, Z: %.2f mm\n", pos_x, pos_y, pos_z);
  } else if (key_u) { // unset origin

    origin_set = false;

  } else if (key_h) { // sets origin

    x_origin = pos_x;
    y_origin = pos_y;
    z_origin = pos_z;
    origin_set = true;
    
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
      }
    }
  }

    if (move) {
      mm = 1.5;
    set_stepper_direction(); // sets the direction 
    mm_to_steps(); // converts the mm to steps
    execute_n_steps(); // executes the steps
    } 

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
        case 'u': case 'U':
        key_u = true;
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
        case '7':
        step_size = 1.5;
        printf("regular mode\n");
        break;
        case '8':
        step_size = 3.0;
        printf("fast mode\n");
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

      buffer_index = 0; // reset buffer index to prevent overflow
      command_buffer[0] = '\0'; // clear the buffer
     }
    }
  }
  if (manual_mode && getchar_timeout_us(0) == PICO_ERROR_TIMEOUT) {
    // resets key states 
        key_w = key_s = key_a = key_d = key_q = key_e = key_o = key_p = key_l = key_h = key_r = false;
  }
}


// function to process and execute the command from the buffer
void process_command() {

  // checks if the command is a G-code or M-code command and prcocesses accoedinly
  if (command_buffer[0] == 'G' || command_buffer[0] == 'g') {
    process_gcode_command(command_buffer); // function call to process G-code command
    return;
  } else if ((command_buffer[0] == 'M' || command_buffer[0] == 'm') && command_buffer[1] != 'a') {
    process_gcode_command(command_buffer); // function call to process M code for spindle
    return;
} else {

  // arrays to store different types of commands
  char command[10];
  char value_str[10];
  char integer[10] = {0}; // array to store integer value as string for parsing

  // uses sscanf to parse the commend and value from the buffer
  int count = sscanf(command_buffer, "%s %s %s", command, value_str, integer);

  // checks if the commend is valid and executes the corresponding command
  if (count >= 1 && default_mode == true) {

    if (strcmp(command, "delay") == 0 && count == 2) {

      int delay_value = 0; // variable to store the delay value
       sscanf(value_str, "%d", &delay_value); // converts the integer value from string to integer

       delay = delay_value; // sets the delay to user input

    } else if (strcmp(command, "axis") == 0 && count == 2) {

      axis_selection = value_str[0]; // sets the axis selection to the value from the commend

    } else if (strcmp(command, "mode") == 0 && count == 2) {

      sscanf(value_str, "%d", &mode); // converts the value from string to integer
      set_microstepping_mode(); // function call to set the microstepping mode based on the value from the commend

    } else if (strcmp(command, "fwd") == 0 && count == 2) {

      forward = true; // sets the direction to forward
      set_stepper_direction(); // sets the direction to forward
      sscanf(value_str, "%d", &steps); // converts the value from string to integer
      execute_n_steps(); // function call to execute the number of steps from the commend

    } else if (strcmp(command, "rev") == 0 && count == 2) {

      forward = false; // sets the direction to reverse
      set_stepper_direction(); // sets the direction to reverse
      sscanf(value_str, "%d", &steps); // converts the value from string to integer
      execute_n_steps(); // function call to execute the number of steps from the commend

    } else if (strcmp(command, "spin") == 0 && count == 2) { 

      sscanf(value_str, "%d", &speed); // saves user input into speed
      
      // error handling for speed inputs
      if (speed < 0 || speed > 100) {

        return;

      }

      // sets the spindle speed to user input percentage
      spindle_speed = (65535 * speed) / 100;
      spindle_control(); 

    } else if (strcmp(command, "help") == 0) {  
     /////////////////////// REMOVE ONCE GUI HAS HELP OPTIONS
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
      delay = 400; // set delay to a fast speed for smooth movement
  
  } else {

    return;
  }
 }
}
}

int main(void) { 

  stdio_init_all();

  sleep_ms(2000); // delay to allow time for serial to connect

  // function calls to initalize pins 
  init_stepper_pins(); 
  init_spindle_motor();
 
  while (true) {

    // function call to process user input
      process_input();

    if (manual_mode) {

    // function call to execute movement based on key states
    execute_manual_movement();

  } else {

    // checks if command is complete
    if (command_complete) {

      process_command(); // function call to process the commend

      // reset buffer and index for next command
      memset(command_buffer, 0, buffer_size); // clear the command buffer
      buffer_index = 0;
      command_complete = false;

    }
  }
} 
  return 0;
}