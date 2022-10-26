#include <SoftwareSerial.h>
int state;
#define ledPin 7


void setup() 
{
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(9600); 
}

void loop() 
{
  if(Serial.available() > 0)
  { 
   state = Serial.read(); 
  }
 if (state == '0') 
 {
  digitalWrite(ledPin, LOW);
  Serial.println("LED: OFF"); 
  state = 0;
 }
 else if (state == '1') 
 {
  digitalWrite(ledPin, HIGH);
  Serial.println("LED: ON");;
  state = 0;
 }
}
