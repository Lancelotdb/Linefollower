
int BIN_1 = 3;
int BIN_2 = 5;
int AIN_1 = 6;
int AIN_2 = 9;
int sleep = 12;
char speed;
void setup() {
    pinMode(BIN_1, OUTPUT);
    pinMode(BIN_2, OUTPUT);
    pinMode(AIN_1, OUTPUT);
    pinMode(AIN_2, OUTPUT);
    pinMode(sleep, OUTPUT);
    Serial.begin(9600);
    digitalWrite(sleep, 1);
    
}

void loop() 
{
  


    if (Serial.available()) 
    {
      speed = Serial.read();

        if (speed == '0')
      {
      Serial.print("Stand ");
      Serial.println(speed);
      Serial.println("Snelheid 0");
      digitalWrite(BIN_2, LOW);
      digitalWrite(AIN_2, LOW);
      analogWrite(AIN_1, 0);
      analogWrite(BIN_1, 0);
      }
       else if(speed == '1')
      {
      Serial.print("Stand ");
      Serial.println(speed);
      Serial.println("Snelheid 25");
      digitalWrite(BIN_2, LOW);
      digitalWrite(AIN_2, LOW);
      analogWrite(AIN_1, 25);
      analogWrite(BIN_1, 25);
      }
       else if(speed == '2')
      {
      Serial.print("Stand ");
      Serial.println(speed);
      Serial.println("Snelheid 50");
      digitalWrite(BIN_2, LOW);
      digitalWrite(AIN_2, LOW);
      analogWrite(AIN_1, 50);
      analogWrite(BIN_1, 50);
      }
       else if(speed == '3')
      {
      Serial.print("Stand ");
      Serial.println(speed);
      Serial.println("Snelheid 100");
      digitalWrite(BIN_2, LOW);
      digitalWrite(AIN_2, LOW);
      analogWrite(AIN_1, 100);
      analogWrite(BIN_1, 100);
      }
       else if(speed == '4')
      {
      Serial.print("Stand ");
      Serial.println(speed);
      Serial.println("Snelheid 150");
      digitalWrite(BIN_2, LOW);
      digitalWrite(AIN_2, LOW);
      analogWrite(AIN_1, 150);
      analogWrite(BIN_1, 150);
      }
       else if(speed == '5')
      {
      Serial.print("Stand ");
      Serial.println(speed);
      Serial.println("Snelheid 200");
      digitalWrite(BIN_2, LOW);
      digitalWrite(AIN_2, LOW);
      analogWrite(AIN_1, 200);
      analogWrite(BIN_1, 200);
      }




    }
 

}