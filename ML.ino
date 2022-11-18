#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

#define n_features 2
#define samples 1

LiquidCrystal_I2C lcd(0x3F, 16, 2);

double duration, inches;
int result, percentage;
bool state, pump, result_pump;
float f_inches, before, diff, set_val, bg_prc;

float X[] = {0, 0};
float w[] = {1.5181657, 1.8601177};
float b=-1.022256;
float tmp[2];

byte Check[8] = {
  0b00000,
  0b00001,
  0b00011,
  0b10110,
  0b11100,
  0b01000,
  0b00000,
  0b00000
};


void setup() {
  
  lcd.begin();
  lcd.backlight();

  Serial.begin(9600);
  before = -1;
  result_pump = 0;
  
  lcd.print("WATER LEVEL:");
  lcd.setCursor(0, 1); 
  lcd.print("PUMP:OFF MANUAL");
  
  pinMode(4, OUTPUT); //In 3 Motor
  pinMode(5, OUTPUT); //In 4 Motor
  pinMode(7, OUTPUT); // ML LED (Red)
  pinMode(8, OUTPUT); // Ultrasonic
  pinMode(9, INPUT); // Ultrasonic
  pinMode(10, INPUT_PULLUP); // Pushbutton
  pinMode(11, INPUT_PULLUP); // Switch
  pinMode(12, OUTPUT); // Motor LED (Green)
  
  set_val=EEPROM.read(0);
  lcd.createChar(0, Check);

  if(set_val>150)set_val=150;
}

void loop() {
  
   digitalWrite(3, LOW);
   delayMicroseconds(2);
   digitalWrite(8, HIGH);
   delayMicroseconds(10);
   digitalWrite(8, LOW);
   duration = pulseIn(9, HIGH);
   inches = microToInches(duration) - 1; 

   f_inches = microToInches(duration);
   diff = before - f_inches;
   if ((diff) < 0) diff = 0;
   before = f_inches;

   percentage=round(((set_val-inches)*100)/set_val);
   bg_prc = percentage;
   lcd.setCursor(12, 0); 
   if(percentage<0)percentage=0;
   if(percentage>100 or inches > 200)percentage=100;
   lcd.print(percentage);
   lcd.print("%   ");
    
   if(percentage<30&digitalRead(11)){
     pump=1;
     digitalWrite(4, LOW);
     digitalWrite(5, HIGH);
    };
   if(percentage>99 or inches > 200){
     pump=0;
     digitalWrite(5, LOW);
//
//     lcd.setCursor(14, 1);
//     lcd.write(0);
   };
   digitalWrite(12,!pump);
     
   lcd.setCursor(5, 1);
   if(pump==1)lcd.print("ON ");
   else if(pump==0)lcd.print("OFF");
   
   lcd.setCursor(9, 1);
   if(!digitalRead(11))lcd.print("MANUAL");
   else lcd.print("AUTO   ");

  if (percentage == 100) {
    lcd.setCursor(15, 1);
    lcd.write(0);
  }
  else {
    lcd.setCursor(15, 1);
    lcd.print(' ');
  };

  if (diff<0.01 or percentage==100) result = 0;
  else {
    X[0] = diff;
    X[1] = (bg_prc/100);
    result = predict_data();
  }

  if (result==0) {
    digitalWrite(7, LOW);
    result_pump = 0;
  }
  else if (result==1 & percentage>=90) {
    result_pump = !result_pump;
    digitalWrite(7, result_pump);
  };
    
    if(!digitalRead(10)&!state&digitalRead(11)){
      state=1;
      set_val=inches;
      EEPROM.write(0, set_val);
    }
      
    if(!digitalRead(10)&!state&!digitalRead(11)){
       state=1;
       pump=!pump; 
    }
      
    if(digitalRead(10))state=0;

  if(before!=-1.00){
    Serial.println(result_pump);
    Serial.print("Distance: ");
    Serial.println(inches);
    Serial.print("Distance reference: ");
    Serial.println(set_val);
    Serial.print("Before: ");
    Serial.println(before);
    Serial.print("Current: ");
    Serial.println(f_inches);
    Serial.print("Before - Current: ");
    Serial.println(diff);
    Serial.print("Percentage: ");
    Serial.println(percentage);
    Serial.print("Result: ");
    Serial.println(result);
    Serial.println("------------------------");
  }

  delay(100);
}

float microToInches(double m) {
   return (0.01723*m)/2.54;
}

float sigmoid(float x){
   double y = 1/(1+exp(-x));
   return y;
}

float neuron(float W[],float B, float X[],int nn){
   float z,y = 0.0;
   for(int i=0; i < nn; i++){
      y += W[i]*X[i];
   }
   y += B;
   z = sigmoid(y);
   return z;
}

int predict_data(){
  float y[samples];
  float t2=0;
  int j=0;

  for(int i=0; i< samples; i++)
  {
    j = n_features*i;
    tmp[0] = X[j];
    tmp[1] = X[j+1];
    y[i] = neuron(w,b,tmp,n_features);

    if(y[i]>0.5)y[i]=1;
    else y[i]=0;

    return y[i];
  }
}