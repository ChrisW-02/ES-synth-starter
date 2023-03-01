#include <Arduino.h>
#include <U8g2lib.h>


//Constants
  const uint32_t interval = 100; //Display update interval
  volatile int32_t currentStepSize;
  volatile uint8_t keyArray[7];

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
  int32_t step = pow(2, 32)*frequency/22000;
  return step;
}

void setRow(uint8_t rowIdx){
  digitalWrite(REN_PIN, LOW);
  digitalWrite(RA0_PIN, bitRead(rowIdx,0));
  digitalWrite(RA1_PIN, bitRead(rowIdx,1));
  digitalWrite(RA2_PIN, bitRead(rowIdx,2));
  digitalWrite(REN_PIN, HIGH);
}

int mapindex(volatile uint8_t keyarray[], int rowindex){

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


void sampleISR(){

  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  int32_t Vout = (phaseAcc >> 24) - 128;
  analogWrite(OUTR_PIN, Vout + 128);

}

void scanKeysTask(void* pvParameters){
  const int32_t keys[13] = {fss(261.63),fss(277.18),fss(293.66),fss(311.13),fss(329.63),fss(349.23),fss(369.99),fss(392),fss(415.3),fss(440),fss(466.16),fss(493.88),0};
  //volatile int32_t currentStepSize;
  //uint8_t keyArray[7];
  int32_t stepSizes;

    for(int i=0; i<3; i++){
      uint8_t rowindex = i;
      setRow(rowindex);
      delayMicroseconds(3);
      keyArray[i] = readCols();
      
    }     
    

    for(int i=0;i<3;i++){

      if(mapindex(keyArray, i)==0){
        u8g2.drawStr(2,10,"C");
      }
      else if(mapindex(keyArray, i)==1){
        u8g2.drawStr(2,10,"C#");
      }
      else if(mapindex(keyArray, i)==2){
        u8g2.drawStr(2,10,"D");
      }
      else if(mapindex(keyArray, i)==3){
        u8g2.drawStr(2,10,"D#");
      }
      else if(mapindex(keyArray, i)==4){
        u8g2.drawStr(2,10,"E");
      }
      else if(mapindex(keyArray, i)==5){
        u8g2.drawStr(2,10,"F");
      }
      else if(mapindex(keyArray, i)==6){
        u8g2.drawStr(2,10,"F#");
      }
      else if(mapindex(keyArray, i)==7){
        u8g2.drawStr(2,10,"G");
      }
      else if(mapindex(keyArray, i)==8){
        u8g2.drawStr(2,10,"G#");
      }
      else if(mapindex(keyArray, i)==9){
        u8g2.drawStr(2,10,"A");
      }
      else if(mapindex(keyArray, i)==10){
        u8g2.drawStr(2,10,"A#");
      }
      else if(mapindex(keyArray, i)==11){
        u8g2.drawStr(2,10,"B");
      }
      else if(mapindex(keyArray, i)==12){
        u8g2.drawStr(2,10," ");
        stepSizes = stepSizes;
      }

      if(mapindex(keyArray, i)!=12){

        u8g2.setCursor(2,20);
        //u8g2.print(stepSizes[mapindex(keyArray, i)]);
        u8g2.print(i);

        stepSizes = keys[mapindex(keyArray, i)];

        u8g2.setCursor(2,30);
        u8g2.print(stepSizes);
      }

      //currentStepSize = stepSizes;
      __atomic_store_n(&currentStepSize, stepSizes, __ATOMIC_RELAXED);

    }
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

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");

  //creat a timer
  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);
  sampleTimer->setOverflow(22000, HERTZ_FORMAT);
  sampleTimer->attachInterrupt(sampleISR);
  sampleTimer->resume();
}

void loop() {
  // put your main code here, to run repeatedly:
  //int index = 12;
  static uint32_t next = millis();
  static uint32_t count = 0;
  //const int32_t keys[13] = {fss(261.63),fss(277.18),fss(293.66),fss(311.13),fss(329.63),fss(349.23),fss(369.99),fss(392),fss(415.3),fss(440),fss(466.16),fss(493.88),0};
  //volatile int32_t currentStepSize;
  //uint8_t keyArray[7];
  //int32_t stepSizes;
  
  if (millis() > next) {
    next += interval;

    //Update display
    u8g2.clearBuffer();         // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    //u8g2.drawStr(2,10,"Helllo World!");  // write something to the internal memory
    //u8g2.setCursor(2,20);
    //u8g2.print(count++);
    //u8g2.sendBuffer();          // transfer internal memory to the display
    /*
    for(int i=0; i<3; i++){
      uint8_t rowindex = i;
      setRow(rowindex);
      delayMicroseconds(3);
      keyArray[i] = readCols();
      
    }     
    

    for(int i=0;i<3;i++){

      if(mapindex(keyArray, i)==0){
        u8g2.drawStr(2,10,"C");
      }
      else if(mapindex(keyArray, i)==1){
        u8g2.drawStr(2,10,"C#");
      }
      else if(mapindex(keyArray, i)==2){
        u8g2.drawStr(2,10,"D");
      }
      else if(mapindex(keyArray, i)==3){
        u8g2.drawStr(2,10,"D#");
      }
      else if(mapindex(keyArray, i)==4){
        u8g2.drawStr(2,10,"E");
      }
      else if(mapindex(keyArray, i)==5){
        u8g2.drawStr(2,10,"F");
      }
      else if(mapindex(keyArray, i)==6){
        u8g2.drawStr(2,10,"F#");
      }
      else if(mapindex(keyArray, i)==7){
        u8g2.drawStr(2,10,"G");
      }
      else if(mapindex(keyArray, i)==8){
        u8g2.drawStr(2,10,"G#");
      }
      else if(mapindex(keyArray, i)==9){
        u8g2.drawStr(2,10,"A");
      }
      else if(mapindex(keyArray, i)==10){
        u8g2.drawStr(2,10,"A#");
      }
      else if(mapindex(keyArray, i)==11){
        u8g2.drawStr(2,10,"B");
      }
      else if(mapindex(keyArray, i)==12){
        u8g2.drawStr(2,10," ");
        stepSizes = stepSizes;
      }

      if(mapindex(keyArray, i)!=12){

        u8g2.setCursor(2,20);
        //u8g2.print(stepSizes[mapindex(keyArray, i)]);
        u8g2.print(i);

        stepSizes = keys[mapindex(keyArray, i)];

        u8g2.setCursor(2,30);
        u8g2.print(stepSizes);
      }

      //currentStepSize = stepSizes;
      __atomic_store_n(&currentStepSize, stepSizes, __ATOMIC_RELAXED);

    }*/

    uint32_t* k = NULL;

    scanKeysTask(k);

    u8g2.sendBuffer();     
    //currentStepSize = stepSizes[index];
    
    //Toggle LED
    digitalToggle(LED_BUILTIN);
  }
}