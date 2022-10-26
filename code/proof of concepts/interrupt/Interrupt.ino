#define LED_PIN 2
#define BUTTON_PIN 3

volatile byte StatusLed = LOW;
void setup() 
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), blink, RISING);
}

void loop() 
{
  digitalWrite(LED_PIN, StatusLed);
}

void blink() 
{
  StatusLed = !StatusLed;
  
}