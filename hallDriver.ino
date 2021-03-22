#include <Wire.h>
#include <Stepper.h>
#include <math.h>
#include <stdbool.h>

const int SW_pin = 2; // digital pin connected to switch output
const int X_pin = A0; // analog pin connected to joystick X output
const int Y_pin = A1; // analog pin connected to joystick Y output

const int stepsPerRevolution = 2048;  // 2048 is the stepsPerRevolution for current motor selection
const int rolePerMinute = 15;         // Adjustable range of 28BYJ-48 stepper is 0~17 rpm

// Define stepper i/o ports
Stepper Stepper_X(stepsPerRevolution, 8, 10, 9, 11);
Stepper Stepper_Y(stepsPerRevolution, 4, 6, 5, 7);

// i2c address of Hall effect sensor
int deviceAddress = 0x60;



//Error Definitions
#define SUCCESS 0
#define TOO_LONG 1
#define ADDRESS_NACK 3
#define DATA_NACK 4
#define OTHER_ERR 5

//Mode Definitions
#define MANUAL 0
#define AUTO 1
#define OFF 2

// Global variables
bool calibrated = false;
int min_X, max_X, min_Y, max_Y;
int cur_X, cur_Y;

int mode = OFF;

bool pass = true;

// Initial setup code
void setup() {
  // i/o definitions for joystick button
  pinMode(SW_pin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  
  Stepper_X.setSpeed(rolePerMinute);
  Stepper_Y.setSpeed(rolePerMinute);

  // Enables i2c communication
  Wire.begin();
  Wire.setClock(10000000);

  
  Serial.begin(115200);

  // waits for serial to be connected
  while (!Serial);
  int n = 0;

  // Put Hall effect sensor into Customer Access mode
  uint16_t error = 0;
  do {
        uint32_t value = 0x2C413534;
        Wire.beginTransmission(deviceAddress);
        Wire.write(0x35);
        Wire.write((byte)(value >> 24));
        Wire.write((byte)(value >> 16));
        Wire.write((byte)(value >> 8));
        Wire.write((byte)(value));    
        error =  Wire.endTransmission();
    if (error != SUCCESS)
      {
          Serial.print("ERROR ");
          Serial.println(error);
          pass = false;
          n++;
      }
      else {
        pass = true;
      }
  } while (error !=SUCCESS && n < 2);
  if (pass) {
    Serial.println("CONNECTED");
  }
}

void loop() {
  command_handler();

  //Manual mode lets the user control the movement of the Hall effect sensor using the joystick
  if (mode == MANUAL) {
    int x_move = -1 * (analogRead(X_pin) - 512);
    int y_move = analogRead(Y_pin) - 512;
    int dir_x = 0;
    int dir_y = 0;
    if (x_move > 30) {
      dir_x = 1;
    }
    else if (x_move <  -30) {
      dir_x = -1;
    }
      if (y_move > 30) {
      dir_y = 1;
    }
    else if (y_move < -30) {
      dir_y = -1;
    }
    Stepper_X.step(dir_x * 100);
    Stepper_Y.step(dir_y * 100);
    if (calibrated) {
      cur_X += 100 * dir_x;
      cur_Y += 100 * dir_y;
    }
  }
  else if (mode == AUTO) {
    // Can add functionality in here, but AUTO mode is handled on the computer side through Serial commands
  }

  
  delay(100);
  
}

// Gets the Magnetic field intensity in the x, y, z directions, adapted from: https://github.com/vintlabs/ESP32-Arduino-3dHall-Example/blob/master/ESP32-Arduino-3dHall-Example.ino
void get_x_y_z(int busAddress, float *X, float *Y, float *Z) {
  uint32_t value0x27;
  uint16_t error = read(busAddress, 0x27, value0x27);
    if (error != SUCCESS)
    {
        Serial.print("ERROR ");
        Serial.println(error);
    }

  value0x27 = (value0x27 & 0xFFFFFFF3) | (0x0 << 2);
  error = write(busAddress, 0x27, value0x27);
    if (error != SUCCESS)
    {
        Serial.print("ERROR ");
        Serial.println(error);
    }

  Wire.beginTransmission(busAddress);
  Wire.write(0x28);
  error = Wire.endTransmission(false);

  if (error == SUCCESS) {
    
    /*Wire.requestFrom(busAddress, 4);
    int x = SignExtendBitfield(Wire.read(), 8);
    int y = SignExtendBitfield(Wire.read(), 8);
    int z = SignExtendBitfield(Wire.read(), 8);
    Wire.read();*/

    Wire.requestFrom(busAddress, 8);
    uint32_t value0x28 = Wire.read() << 24;
    value0x28 += Wire.read() << 16;
    value0x28 += Wire.read() << 8;
    value0x28 += Wire.read();

    uint32_t value0x29 = Wire.read() << 24;
    value0x29 += Wire.read() << 16;
    value0x29 += Wire.read() << 8;
    value0x29 += Wire.read();

    int x = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
    int y = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
    int z = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

    
    *X = (float)x / 4.0;
    *Y = (float)y / 4.0;
    *Z = (float)z / 4.0;

    
  }
  else {
    Serial.print("ERROR ");
    Serial.println(error);
  }
}

// Adapted from: https://github.com/vintlabs/ESP32-Arduino-3dHall-Example/blob/master/ESP32-Arduino-3dHall-Example.ino
uint16_t read(int busAddress, uint8_t address, uint32_t& value)
{
    // Write the address that is to be read to the device
    Wire.beginTransmission(busAddress);
    Wire.write(address);
    int error = Wire.endTransmission(false);

    // if the device accepted the address,
    // request 4 bytes from the device
    // and then read them, MSB first
    if (error == 0)
    {
        Wire.requestFrom(busAddress, 4);
        value = Wire.read() << 24;
        value += Wire.read() << 16;
        value += Wire.read() << 8;
        value += Wire.read();
    }

    return error;
}


// Adapted from: https://github.com/vintlabs/ESP32-Arduino-3dHall-Example/blob/master/ESP32-Arduino-3dHall-Example.ino
uint16_t write(int busAddress, uint8_t address, uint32_t value)
{
    // Write the address that is to be written to the device
    // and then the 4 bytes of data, MSB first
    Wire.beginTransmission(busAddress);
    Wire.write(address);
    Wire.write((byte)(value >> 24));
    Wire.write((byte)(value >> 16));
    Wire.write((byte)(value >> 8));
    Wire.write((byte)(value));    
    return Wire.endTransmission();
}

// Adapted from: https://github.com/vintlabs/ESP32-Arduino-3dHall-Example/blob/master/ESP32-Arduino-3dHall-Example.ino
long SignExtendBitfield(uint32_t data, int width)
{
    long x = (long)data;
    long mask = 1L << (width - 1);

    if (width < 32)
    {
        x = x & ((1 << width) - 1); // make sure the upper bits are zero
    }

    return (long)((x ^ mask) - mask);
}

// Calibrates the position of the Hall effect sensor by using the joystick to move the sensor between two opposing corners
void calibration() {
  // c1 and c2 are the calibration states of both corners
  bool c1 = false;
  bool c2 = false;

  // Initial temporary position set to current location
  cur_X = 0;
  cur_Y = 0;

  
  bool pressed = false;
  Serial.println("READY");

  // Temporary coordinates relative to initial position at beginning of calibration
  int tempMinX, tempMinY, tempMaxX, tempMaxY;
  int count = 0;

  // Until both corners are calibrated
  while (!(c1 && c2)) {
    if (digitalRead(SW_pin) == LOW) {
      // Ensure that a long joystick press does not get registered as multiple presses and account for signal variations
      int x_move = analogRead(X_pin) - 512;
      int y_move = analogRead(Y_pin) - 512;
      if (!pressed && count >= 20 && abs(x_move) < 20 && abs(y_move) < 20) {
        pressed = true;
        count = 0;
        if (!c1) {
          c1 = true;
          tempMinX = cur_X;
          tempMinY = cur_Y; 
          Serial.println("MIN_SET");
        }
        else if (!c2) {
          c2 = true;
          tempMaxX = cur_X;
          tempMaxY = cur_Y;
          Serial.println("MAX_SET");
        }
      }
      else if (!pressed) {
        count += 1;
      }
    }
    else {
      // Move the sensor using the joystick
      pressed = false;
      int x_move = -1 * (analogRead(X_pin) - 512);
      int y_move = analogRead(Y_pin) - 512;
      count = 0;
      int dir_x = 0;
      int dir_y = 0;
      if (x_move > 20) {
        dir_x = 1;
      }
      else if (x_move <  -20) {
        dir_x = -1;
      }
        if (y_move > 20) {
        dir_y = 1;
      }
      else if (y_move < -20) {
        dir_y = -1;
      }
      Stepper_X.step(dir_x * 100);
      Stepper_Y.step(dir_y * 100);
      cur_X += dir_x * 100;
      cur_Y += dir_y * 100;
    }
    delay(10);
  }
  // Change of coordinates
  min_X = 0;
  max_X = max(tempMinX, tempMaxX) - min(tempMinX, tempMaxX);
  min_Y = 0;
  max_Y = max(tempMinY, tempMaxY) - min(tempMinY, tempMaxY);
  cur_X = cur_X - min(tempMinX, tempMaxX);
  cur_Y = cur_Y - min(tempMinY, tempMaxY);
  Serial.println("DONE");
  calibrated = true;
}

// Go to (0, 0)
void go_home() {
  go_to(min_X, min_Y);
}


// Moves Hall effect sensor to requested x and y
void go_to(int x, int y) {
  if (calibrated) {
    int x_move = cur_X - x;
    Stepper_X.step(x_move);
    int y_move = cur_Y - y;
    Stepper_Y.step(y_move);
    cur_X = x;
    cur_Y = y;
    Serial.println("DONE");
  }
  else {
    Serial.println("UNCALIBRATED");
  }
}


// Get current x and y
void get_pos() {
  if (calibrated) {
    Serial.print(cur_X);
    Serial.print(", ");
    Serial.println(cur_Y);
  }
  else {
    Serial.println("UNCALIBRATED");
  }
}

// Get calibrated boundary coordinates
void get_bounds() {
  if (calibrated) {
    Serial.print(min_X);
    Serial.print(", ");
    Serial.print(max_X);
    Serial.print(", ");
    Serial.print(min_Y);
    Serial.print(", ");
    Serial.println(max_Y);
  }
  else {
    Serial.println("UNCALIBRATED");
  }
}

// Change between manual and auto mode
void change_mode(int _mode) {
  mode = _mode;
  //digitalWrite(LED_BUILTIN, HIGH);
  //delay(500);
  //digitalWrite(LED_BUILTIN, LOW);
  Serial.println("DONE");
}

// Get whether the robot is calibrated
void get_calibrated() {
  if (calibrated) {
    Serial.println("TRUE");
  }
  else {
    Serial.println("FALSE");
  }
}

// Display the measured magnetic field
void get_intensity() {
  float X, Y, Z;
  get_x_y_z(deviceAddress, &X, &Y, &Z);
  Serial.print(X);
  Serial.print(", ");
  Serial.print(Y);
  Serial.print(", ");
  Serial.println(Z);
}

// Takes in a serial command, parses it, and calls the correct function
void command_handler() {
  String cmdString;
  bool wordFound = false;
  while(Serial.available()) {
    delay(3);
    if (Serial.available() > 0) {
      char c = Serial.read();
      cmdString += c;
    }
    wordFound = true;
  }
  
  if (wordFound) {
    char *cmdChar = (char*)malloc(cmdString.length()+1);
    cmdString.toCharArray(cmdChar, cmdString.length()+1);
    if (strcmp(cmdChar, "GET_POS") == 0) {
      get_pos();
    }
    else if (strcmp(cmdChar, "CALIBRATE") == 0) {
      calibration();
    }
    else if (strcmp(cmdChar, "GET_BOUNDS") == 0) {
      get_bounds();
    }
    else if (strcmp(cmdChar, "GET_CALIBRATED") == 0) {
      get_calibrated();
    }
    else if (strcmp(cmdChar, "GET_RESULTS") == 0) {
      get_intensity();
    }
    else {
      
      char* cmdPart = strtok(cmdChar, " ");
      if (cmdPart != NULL) {  
        if (strcmp(cmdPart, "GO_TO") == 0) {
          int x;
          int y;
          cmdPart = strtok(NULL, "  ");
          if (cmdPart != NULL) {
            sscanf(cmdPart, "%d", &x);
            cmdPart = strtok(NULL, "  ");
            if (cmdPart != NULL) {
              sscanf(cmdPart, "%d", &y);
              go_to(x, y);
            }
          }
        }
        else if (strcmp(cmdPart, "CHANGE_MODE") == 0) {
          cmdPart = strtok(NULL, "  ");
          if (cmdPart != NULL) {
            if (strcmp(cmdPart, "MANUAL") == 0) {
              change_mode(MANUAL);
            }
            else if (strcmp(cmdPart, "AUTO") == 0) {
              change_mode(AUTO);
            }
            else if (strcmp(cmdPart, "OFF") == 0) {
              change_mode(OFF); 
            }
          }
        }
      } 
    }
    free(cmdChar);
  }
}
