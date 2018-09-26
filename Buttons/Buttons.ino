boolean button;
long startTime;
long elapsedTime;

void setup() {
  Serial.begin(9600);
  //configure pin2 as an input and enable the internal pull-up resistor
  pinMode(3, INPUT_PULLUP);
}

void loop() {
  button = digitalRead(3);

  startTime = millis();
  long endTime = 0;
  elapsedTime = 0;
  while ((button==0)&&(elapsedTime<1010))
  {
    button = digitalRead(3);
    elapsedTime = millis() - startTime;
  }
  
  if (elapsedTime>1000)
  {
    Serial.println(">1 sec: OK!");
  }
  
  delay(100);
}
