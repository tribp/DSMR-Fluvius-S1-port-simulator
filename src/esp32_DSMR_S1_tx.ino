
#define RXD2 16
#define TXD2 17

//Uncommend for real-time
#define TELEGRAMTIME   370000 // REAL-TIME mode: Subtract 15000 for correction factor - 385 us = 2600Hz sample freq

//Uncommend to debug
//#define TELEGRAMTIME   20000000 // Set DEBUG mode 2 sec interval per telegram (2.000.000.000) or 0.02 sec for DFT  (200.000)




/*E-MUCS Frame = 45 bytes
 *  Flag          = 1 byte = 0x7E
 *  Frame Format  = 2 bytes = Frame Type (byte 1=0x80) + Frame Lenght (byte 2 = fixed = 0x2B = 43 bytes
 *  Address Field = 1 byte = broadcast to all stations = 0xFF
 *  Control Field = 1 byte = 0x03
 *  Data          = 37 bytes
 *      Meter ID        = byte 6 to 19
 *      Additional Info = byte 20
 *        bit 0 = meter type (0 = single fase 1 = 3 fase)
 *        bit 1 = sampling type (per sec = O , per period =1) 
 *        bit 2 = (3-wire = 0 , 4 wire = 1) only when 3 fase
 *        bit 3 = samples ok ( 0=corrupted, 1 = ok)
 *        bit 4 = neutral current measured ( 0=no, 1=yes)
 *        bit 5-7 = Dataformat version (default '000')
 *     Sampling freq = byte 21
 *        if fixed per second = multiples of 100hz
 *        if per period = number of samples for 1 period (can vary)
 *     Network freq = byte 22-23 = uInt in mHz (0xC3 0xBB = 50.096 Hz)
 *     Frame Seq Nr = byte 24
 *     Voltage L1 = byte 25-26 in 25 mV
 *        Voltage L1-MSB = byte 25
 *        Voltage L1-LSB = byte 26
 *     Current L1 = byte 27-29
 *        Current L1-MSB = byte 27
 *        Current L1     = byte 28
 *        Current L1-LSB = byte 29
 *     Voltage L2 = byte 30-31
 *     Current L2 = byte 32-34
 *     Voltage L3 = byte 35-36
 *     Current L3 = byte 37-39
 *     Current N  = byte 40-42
 *     FCS        = byte 43-44
 *        FCS-LSB = byte 43
 *        FCS-MSB = 44
 *    Flag (Closing)  = 1 byte
 */

#define Flag  0x7E;      //tilde
#define FrameType 0x80;
#define FrameLength 0x2B;
#define Address  0xFF;
#define ControlField  0x03;
  

byte AddInfo = 0x0A;    //0000 1010 = Single fase / samples per period / samples OK
byte SampleFreq = 0x34;  // 34 hex = 52 samples per 50hz ( = 2600 Hz sample freq)
byte NetworkFreqMSB = 0xC3;
byte NetworkFreqLSB = 0xBB;
byte FrameSeq = 0x01;   // eg Frame 1
byte VoltageL1MSB = 0x0B;   //? eg: 0x0B = 11 V + 0x15 = 21 x 25mV = 525 mV -> 11.525mV 
byte VoltageL1LSB = 0x15;   //? eg: 0B 16 = 2837 x 25mV = 70.925 mV - for this sample

//test
int cycles;           // for measuring time with CPU cycles - 1 cycle = 4 ns
int ellapsedTime;     // in ns
int _startCycles;

// V is coded in 2 bytes cfr eMUCS standard
short Vp = 13011;     //13011 = 230 x sqrt(2) x 1000mV / 25mV  in 25mV (short = 2 bytes) -> OK 2 bytes 
short V;
byte V_MSB;
byte V_LSB;
// I is coded in 3 bytes cfr eMUCS standard
int I;                // int = 4 bytes on esp32 -> need  3 bytes 
int I_test;
byte I_MSB;
byte I_XSB;
byte I_LSB;
int R = 100;        // 100 ohm to make I that is 100 th of voltage for debug purposes.
int sampleNr;       // 52 samples per T

//Example telegram
byte telegram[45];

void initTelegram(){
//Header
  telegram[0] = Flag;
  telegram[1] = FrameType;
  telegram[2] = FrameLength;
  telegram[3] = Address;
  telegram[4] = ControlField;
  telegram[5] = 0x01;  // Meter ID
  telegram[6] = 0x02;
  telegram[7] = 0x03;
  telegram[8] = 0x04;
  telegram[9] = 0x05;
  telegram[10] = 0x06;
  telegram[11] = 0x07;
  telegram[12] = 0x08;
  telegram[13] = 0x09;
  telegram[14] = 0x0A;
  telegram[15] = 0x0B;
  telegram[16] = 0x0C;
  telegram[17] = 0x0D;
  telegram[18] = 0x0E;
  telegram[19] = 0x0A; //Single fase
  telegram[20] = 0x34; //Sampl Freq
  telegram[21] = 0xC3; //Netw Freq MSB
  telegram[22] = 0xBB; //Netw Freq LSB
  telegram[23] = 0x0F; //Frame Sequence
  telegram[24] = 0x0B; //V-L1 MSB
  telegram[25] = 0x15; //V-L1 LSB
  telegram[26] = 0x05; // I-L1 MSB
  telegram[27] = 0x05; // I-L1 
  telegram[28] = 0xFA; // I-L1 LSB
  telegram[29] = 0x05; //V-L2 MSB
  telegram[30] = 0x05; //V-L2 LSB
  telegram[31] = 0x05; // I-L2 MSB
  telegram[32] = 0x05; // I-L2 
  telegram[33] = 0x05; // I-L2 LSB
  telegram[34] = 0x05; //V-L3 MSB
  telegram[35] = 0x05; //V-L3 LSB
  telegram[36] = 0x05; // I-L3 MSB
  telegram[37] = 0x05; // I-L3 
  telegram[38] = 0x05; // I-L3 LSB
  telegram[39] = 0x05; // I-N MSB
  telegram[40] = 0x05; // I-N 
  telegram[41] = 0x05; // I-N LSB
  telegram[42] = 0xC5; // FCS LSB
  telegram[43] = 0xB2; // FCS MSB
  telegram[44] = Flag; // Closing Flag
}

//Gen Variables
int i;

void setup() {
  // put your setup code here, to run once: 
  Serial.begin(115200);
  Serial1.begin(2000000,SERIAL_8N1, RXD2, TXD2);
  initTelegram();
  
  // Init variables
  sampleNr = 0;
  FrameSeq = 0;
  ellapsedTime =0;
  _startCycles = ESP.getCycleCount();

}

void loop() {
  
  // Increment sampleNr each Telegram - starting at 0:
  telegram[23] = FrameSeq;
  if (FrameSeq < 255){
    FrameSeq++;
  }
  else {
    FrameSeq = 0x01;
  }

  // Make test Voltage
  V = Vp * sin(2*PI*sampleNr/52);     // * 1000mV/25mV=40 because must send as multiple of 25mV
  I = V * 25 / 100;                   // in mA - as debug we take R=100 Ohm so I in 1/100th of V.
  V_MSB = (byte)(V>>8);
  V_LSB = (byte)V;
  
  //test value identical as in eMUC spec to check coding-decoding 
  /*
  V_MSB = 0x0B;
  V_LSB = 0x15; */
  
  telegram[24] = V_MSB;
  telegram[25] = V_LSB;

  //Convert I to 3 byte signed Integer - two compliment way
    I_MSB = (byte)(I>>16);
    I_XSB = (byte)(I>>8);
    I_LSB = (byte)I;
    
    //test value identical as in eMUC spec to check coding-decoding 
    /*
    I_MSB = 0x00;
    I_XSB = 0x05;
    I_LSB = 0xFA;
    */
    telegram[26] = I_MSB;
    telegram[27] = I_XSB;
    telegram[28] = I_LSB;

  //Test
  /*
  Serial.print("Voltage:");
  Serial.println(V*25);
  Serial.print("V_MSB + LSB:");
  Serial.println(V_MSB,HEX);
  Serial.println(V_LSB,HEX);
  Serial.print("I:");
  Serial.println(I);
  Serial.println(I,BIN);
  Serial.println("I_MSB + XSB + LSB:");
  Serial.println(I_MSB,BIN);
  Serial.println(I_XSB,BIN);
  Serial.println(I_LSB,BIN);
  */
  
  //Delay for Debugging
  ellapsedTime = (ESP.getCycleCount() - _startCycles)*4;
  
  //delayMicroseconds(415);
  
  //Print Telegram 
  /*
  for (i=0;i<45;i++){
    Serial.print("telegram [");
    Serial.print(i);
    Serial.print("] : ");
    Serial.println(telegram[i],HEX);
  }*/

  //Wait x us  , 1 cycle = 4 ns = 4000us
  while (ellapsedTime < TELEGRAMTIME){        //  = 385 us for RealTime SMsimulation or 2 sec for debugging
    ellapsedTime = (ESP.getCycleCount() - _startCycles)*4;
  }

  //Send Telegram
  _startCycles = ESP.getCycleCount();
  Serial1.write(telegram,45);

  // Increment Sample Nr - inital start = 0
  if (sampleNr < 51){
    sampleNr++;
  }
  else {
    sampleNr = 0;
  }
  
  
  

}
