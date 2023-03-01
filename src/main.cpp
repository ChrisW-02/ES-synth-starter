#include <Arduino.h>
#include <U8g2lib.h>

//Constants
  const uint32_t interval = 100; //Display update interval

// global variable
volatile int32_t currentStepSize;
//volatile uint8_t keyArray[7];

//Pin definitions
  //Row select and enable
  const int RA0_PIN = D3;
  const int RA1_PIN = D6;
  const int RA2_PIN = D12;
  const int REN_PIN = A5;

  //Matrix input and output
  const int C0_PIN = A2;
  const int C1_PIN = D9;
  const int C2_PIN = A6;
  const int C3_PIN = D1;
  const int OUT_PIN = D11;

  //Audio analogue out
  const int OUTL_PIN = A4;
  const int OUTR_PIN = A3;

  //Joystick analogue in
  const int JOYY_PIN = A0;
  const int JOYX_PIN = A1;

  //Output multiplexer bits
  const int DEN_BIT = 3;
  const int DRST_BIT = 4;
  const int HKOW_BIT = 5;
  const int HKOE_BIT = 6;

//Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

//Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value) {
      digitalWrite(REN_PIN,LOW);
      digitalWrite(RA0_PIN, bitIdx & 0x01);
      digitalWrite(RA1_PIN, bitIdx & 0x02);
      digitalWrite(RA2_PIN, bitIdx & 0x04);
      digitalWrite(OUT_PIN,value);
      digitalWrite(REN_PIN,HIGH);
      delayMicroseconds(2);
      digitalWrite(REN_PIN,LOW);
}

uint8_t readCols(){

  int C0 = digitalRead(C0_PIN);
  int C1 = digitalRead(C1_PIN);
  int C2 = digitalRead(C2_PIN);
  int C3 = digitalRead(C3_PIN);

  uint8_t res=0;
  res |= (C0<<3);
  res |= (C1<<2);
  res |= (C2<<1);
  res |= C3;

  return res;
}

int32_t fss(int frequency){
  int32_t step = 2^32*frequency/22000;
  return step;
}

void setRow(uint8_t rowIdx){
  digitalWrite(REN_PIN, LOW);
  digitalWrite(RA0_PIN, bitRead(rowIdx,0));
  digitalWrite(RA1_PIN, bitRead(rowIdx,1));
  digitalWrite(RA2_PIN, bitRead(rowIdx,2));
  digitalWrite(REN_PIN, HIGH);
}

int mapindex(uint8_t keyarray[], int rowindex){

  int index;
 
  if(keyarray[rowindex] == 7){
     index = 0+ rowindex*4;
  }
  else if(keyarray[rowindex] == 11){
    index = 1+ rowindex*4;
  }
  else if(keyarray[rowindex] == 13){
    index = 2+ rowindex*4;
  }
  else if(keyarray[rowindex] == 14){
    index = 3+ rowindex*4;
  }
  else{
    index = 12;
  }
  return index;
}


String mapkey(int index){

  String str = "C";

  if(index==0){
    str = "C";
  }
  else if(index==1){
    str = "C#";
  }
  else if(index==2){
    str = "D";
  }
  else if(index==3){
    str = "D#";
  }
  else if(index==4){
    str = "E";
  }
  else if(index==5){
    str = "F";
  }
  else if(index==6){
    str = "F#";
  }
  else if(index==7){
    str = "G";
  }
  else if(index==8){
    str = "G#";
  }
  else if(index==9){
    str = "A";
  }
  else if(index==10){
    str = "A#";
  }
  else if(index==11){
    str = "B";
  }
  else if(index==12){
    str = "000";
  }

  //char op[] = str;
  return str;
}
void sampleISR() {
  static int32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  int32_t Vout = phaseAcc >> 24;
  analogWrite(OUTR_PIN, Vout + 128);
}

void setup() {
  // put your setup code here, to run once:

  //Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  //Initialise display
  setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);
  sampleTimer->setOverflow(22000, HERTZ_FORMAT);
  sampleTimer->attachInterrupt(sampleISR);
  sampleTimer->resume();

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t next = millis();
  static uint32_t count = 0;
  
  uint8_t keyArray[7];
  //const int32_t keys [] = {static_cast<int>(pow(2,32)*261.63/22000, pow(2,32)*277.18/22000, pow(2,32)*293.66/22000, pow(2,32)*311.13/22000, pow(2,32)*329.63/22000, pow(2,32)*349.23/22000, pow(2,32)*369.99/22000, pow(2,32)*392/22000, pow(2,32)*415.3/22000, pow(2,32)*440/22000, pow(2,32)*466.16/22000, pow(2,32)*493.88/22000)};
  const int32_t keys [] = {51076922,54112683,57330004,60740598,64352275,68178701,72231588,76528508,81077269,85899345,91006452,96418111};
   int32_t stepSizes;
  /*
  for (int i=0;i<12;i++){
    stepSizes [i] = {2^23*keys[i]/22000};
  }*/
  
  /*for(int i = 0; i < 12; i++){
    Serial.println(stepSizes[i]);
  }*/

  if (millis() > next) {
    next += interval;

    //Update display
    u8g2.clearBuffer();         // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    //u8g2.drawStr(2,10,"Helllo World!");  // write something to the internal memory
    //u8g2.setCursor(2,20);
    //u8g2.print(count++);
    //u8g2.sendBuffer();          // transfer internal memory to the display

    for(int i=0; i<12; i++){
      uint8_t rowindex = i;
      setRow(rowindex);
      delayMicroseconds(3);
      keyArray[i] = readCols();
      //u8g2.setCursor(2,20);
      //u8g2.print(keyArray[i],HEX); 
    } 
    

    for(int i=0;i<12;i++){

      if(mapindex(keyArray, i)==0){
        u8g2.drawStr(2,10,"C");
        stepSizes = keys[0];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==1){
        u8g2.drawStr(2,10,"C#");
        stepSizes = keys[1];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==2){
        u8g2.drawStr(2,10,"D");
        stepSizes = keys[2];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==3){
        u8g2.drawStr(2,10,"D#");
        stepSizes = keys[3];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==4){
        u8g2.drawStr(2,10,"E");
        stepSizes = keys[4];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==5){
        u8g2.drawStr(2,10,"F");
        stepSizes = keys[5];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==6){
        u8g2.drawStr(2,10,"F#");
        stepSizes = keys[6];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==7){
        u8g2.drawStr(2,10,"G");
        stepSizes = keys[7];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==8){
        u8g2.drawStr(2,10,"G#");
        stepSizes = keys[8];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==9){
        u8g2.drawStr(2,10,"A");
        stepSizes = keys[9];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==10){
        u8g2.drawStr(2,10,"A#");
        stepSizes = keys[10];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==11){
        u8g2.drawStr(2,10,"B");
        stepSizes = keys[11];
        u8g2.setCursor(2,20);
        u8g2.print(stepSizes,DEC);
      }
      else if(mapindex(keyArray, i)==12){
        u8g2.drawStr(2,10," ");
        stepSizes = stepSizes;
      }
      currentStepSize = stepSizes;
      u8g2.setCursor(2,30);
      u8g2.print(currentStepSize,DEC);

    }

    u8g2.sendBuffer();     
    
    //Toggle LED
    digitalToggle(LED_BUILTIN);
  }
}