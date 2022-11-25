#include <Arduino.h>
#include <SoftwareSerial.h>
#include "SerialCommand.h"
#include "EEPROMAnything.h"

#define SerialPort Serial
#define Baudrate 9600
#define BUTTON_PIN 2
#define LED_PINAL 13
#define LED_PINAR 10
#define LED_PINV 7

int MotorLeftForward = 5;
int MotorLeftBackward = 3;
int MotorRightForward = 6;
int MotorRightBackward = 9;

SerialCommand sCmd(SerialPort);
volatile byte intr = LOW;
int sleep = 12;
bool start;
bool debug;
float iTerm;
float lastErr;
unsigned long previous, calculationTime;
const int sensor[] = {A5, A4, A3, A2, A1, A0};
int normalised[6];
float debugPosition;

struct param_t
{
  unsigned long cycleTime;
  int black[6];
  int white[6];
  int power;
  float diff;
  float kp;
  float ki;
  float kd;
} params;

void onUnknownCommand(char *command);
void onSet();
void onDebug();
void OnCalibrate();
void onStart();
void onStop();
void interrupt();
void onLights();

void setup()
{
  SerialPort.begin(Baudrate);

  // Alle comando's hier:
  sCmd.addCommand("set", onSet);
  sCmd.addCommand("debug", onDebug);
  sCmd.addCommand("calibrate", onCalibrate);
  sCmd.addCommand("start", onStart);
  sCmd.addCommand("stop", onStop);
  sCmd.addCommand("lichtshow", onLights);
  sCmd.setDefaultHandler(onUnknownCommand);

  pinMode(sleep, OUTPUT);
  digitalWrite(sleep, true);
  pinMode(MotorLeftForward, OUTPUT);
  pinMode(MotorLeftBackward, OUTPUT);
  pinMode(MotorRightForward, OUTPUT);
  pinMode(MotorRightBackward, OUTPUT);
  pinMode(LED_PINAL, OUTPUT);
  pinMode(LED_PINAR, OUTPUT);
  pinMode(LED_PINV, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), interrupt, RISING);

  EEPROM_readAnything(0, params);

  SerialPort.println("--- Ready for take off ✈️ ---");
  Serial.println();
  SerialPort.println("To start: ");
  SerialPort.println(" - Type start ");
  SerialPort.println(" - Press start ");
  SerialPort.println("To Stop: ");
  SerialPort.println(" - Type stop ");
  SerialPort.println(" - Press stop ");
}

void loop()
{
  sCmd.readSerial();

  if (start || intr)
  {
    digitalWrite(LED_PINAL, HIGH);
    digitalWrite(LED_PINAR, HIGH);
    digitalWrite(LED_PINV, HIGH);
  }
  else
  {
     digitalWrite(LED_PINAL, LOW);
     digitalWrite(LED_PINAR, LOW);
     digitalWrite(LED_PINV, LOW);
  }
 
  unsigned long current = micros();
  if (current - previous >= params.cycleTime)
  {
    previous = current;

    /* Sensor waardes uitlezen en normaliseren */
    for (int i = 0; i < 6; i++)
    {
      normalised[i]= map(analogRead(sensor[i]), params.black[i], params.white [i], 1, 1000);
    }

    /* Sensorwaardes interpoleren */
    float position = 0;
    int index = 0;
    for (int i = 0; i < 6; ++i) if (normalised [i] < normalised[index]) index = i;

    if (normalised[index] > 750) start = false, intr = false;

    if (index == 0) position = -30;
    else if (index == 5) position = 30;

    else 
    {
      int sNul = normalised[index];
      int sMinEen = normalised[index-1];
      int sPlusEen = normalised[index+1];
    
      float b = sPlusEen - sMinEen;
      b = b / 2;

      float a = sPlusEen - b - sNul;

      position = -b / (2 * a);
      position += index;
      position -= 2.5;

      position *= 15;
    }
    debugPosition = position;
    
    /* Proportioneel regelen */
    float error = -position;
    float output = error* params.kp;

    /* Integrerend regelen*/
    iTerm += params.ki * error;
    iTerm = constrain(iTerm, -510, 510);
    output += iTerm;

    /* Differentiërend regelen */
    output += params.kd * (error - lastErr);
    lastErr = error;

    /* Output begrenzen */
    output = constrain(output, -510, 510);
    
    int powerLeft = 0;
    int powerRight = 0;

    if (start || intr) if (output >= 0)
    {
      powerLeft = constrain(params.power + params.diff * output, -255, 255);
      powerRight = constrain(powerLeft - output, -255, 255);
      powerLeft = powerRight + output;
    }
    else
    {
      powerRight = constrain(params.power - params.diff * output, -255, 255);
      powerLeft = constrain(powerRight + output, -255, 255);
      powerRight = powerLeft - output;
    }

    analogWrite(MotorRightForward, powerLeft > 0 ? powerLeft : 0);
    analogWrite(MotorRightBackward, powerLeft < 0 ? -powerLeft : 0);
    analogWrite(MotorLeftForward, powerRight > 0 ? powerRight : 0);
    analogWrite(MotorLeftBackward, powerRight < 0 ? -powerRight : 0);
  }

  unsigned long difference = micros() - current;
  if (difference > calculationTime) calculationTime = difference;
}

void onUnknownCommand(char *command)
{
  SerialPort.print("Unknown command: \"");
  SerialPort.print(command);
  SerialPort.println("\"");
}

void onSet()
{
 
  digitalWrite(LED_PINV, HIGH);
  delay(1000);
  digitalWrite(LED_PINV, LOW);

  char* param = sCmd.next();
  char* value = sCmd.next();

  if (strcmp(param, "cycle") == 0) 
  {
    long newCycleTime = atol(value);
    float ratio = ((float) newCycleTime) / ((float) params.cycleTime);

    params.ki *= ratio;
    params.kd /= ratio;

    params.cycleTime = newCycleTime;
  }
  else if (strcmp(param, "ki") == 0)
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.ki = atof(value) * cycleTimeInSec;
  }
  else if (strcmp(param, "kd") == 0)
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.kd = atof(value) / cycleTimeInSec;
  }
  else if(strcmp(param, "power") == 0) params.power = atol(value);
  else if(strcmp(param, "diff") == 0) params.diff = atof(value);
  else if(strcmp(param, "kp") == 0) params.kp = atof(value);
  
  EEPROM_writeAnything(0, params);
}


void onDebug()
{
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);

  SerialPort.print("----------Parameters----------");
  SerialPort.println();

  SerialPort.print("black: ");
  for (int i = 0; i < 6; i++)
  {
    SerialPort.print(params.black[i]);
    SerialPort.print(" ");
  }
  SerialPort.println(" "); 

  SerialPort.print("white: ");
  for (int i = 0; i < 6; i++)
  {
    SerialPort.print(params.white[i]);
    SerialPort.print(" ");
  }
  SerialPort.println(" ");
  
  SerialPort.print("normalised: ");
  for (int i = 0; i < 6; i++)
  {
    SerialPort.print(normalised[i]);
    SerialPort.print(" ");
  }
  SerialPort.println(" ");

  SerialPort.print("position: ");
  SerialPort.println(debugPosition);
  Serial.println();

  SerialPort.print("power: ");
  SerialPort.println(params.power);

  SerialPort.print("diff: ");
  SerialPort.println(params.diff);

  SerialPort.print("kp: ");
  SerialPort.println(params.kp); 

  float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
  float ki = params.ki / cycleTimeInSec;
  SerialPort.print("Ki: ");
  SerialPort.println(ki);


  float kd = params.kd * cycleTimeInSec;
  SerialPort.print("Kd: ");
  SerialPort.println(kd);
  Serial.println();
  

  SerialPort.print("cycle time: ");
  SerialPort.println(params.cycleTime);

  SerialPort.print("calculation time: ");
  SerialPort.println(calculationTime);
  calculationTime = 0;

}

void onCalibrate()
{
  char* param = sCmd.next();
 
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  delay(100);
  digitalWrite(LED_PINV, HIGH);


  if (strcmp(param, "black") == 0)
  {
    SerialPort.print("Start calibrating black... ⚫ ");
    for (int i = 0; i < 6; i++) params.black[i] = analogRead(sensor[i]);
    delay(1500);
    SerialPort.println();
    SerialPort.print("Calibrated values: 📊 ");
    for (int i = 0; i < 6; i++)
    {
      SerialPort.print(params.black[i]);
      SerialPort.print(" ");
    }
    SerialPort.println();
  }

  else if (strcmp(param, "white") == 0)
  {
    SerialPort.print("Start calibrating white... ⚪ ");
    for (int i = 0; i < 6; i++) params.white[i] = analogRead(sensor[i]);
    delay(1500);
    SerialPort.println();
    SerialPort.print("Calibrated values: 📊 ");
    for (int i = 0; i < 6; i++)
    {
      SerialPort.print(params.white[i]);
      SerialPort.print(" ");
    }
    SerialPort.println();
  }

  EEPROM_writeAnything(0, params);
}

void onStart()
{
  start = true;
  iTerm = 0;
  SerialPort.print("Started 🏳️");
  SerialPort.println();
}

void onStop()
{
  start = false;
  intr = false;
  
  SerialPort.print("Stopped 🏁");
  SerialPort.println();
}

void interrupt()
{
  intr = !intr;
  if (intr)
  {
    iTerm = 0;
    SerialPort.print("Started 🏳️");
    SerialPort.println();
  }
  else if (!intr)
  {
    start = false;
    SerialPort.print("Stopped 🏁");
    SerialPort.println();
  }

}
void onLights()
{ 

  SerialPort.print("Lichtshow 💡");
  SerialPort.println();
  digitalWrite(LED_PINV, HIGH);
  delay(600);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(600);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(600);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(500);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(500);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(500);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(400);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(400);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(400);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(300);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(300);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(300);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(200);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(200);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(200);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(175);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(175);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(175);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(150);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(150);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(150);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(125);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(125);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(125);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  digitalWrite(LED_PINV, HIGH);
  delay(100);
  digitalWrite(LED_PINV, LOW);
  digitalWrite(LED_PINAR, HIGH);
  delay(100);
  digitalWrite(LED_PINAR, LOW);
  digitalWrite(LED_PINAL, HIGH);
  delay(100);
  digitalWrite(LED_PINAL, LOW);
  delay(2000);


  digitalWrite(LED_PINAR, HIGH);
  digitalWrite(LED_PINAL, HIGH);
  digitalWrite(LED_PINV, HIGH);
  delay(7000);
}





