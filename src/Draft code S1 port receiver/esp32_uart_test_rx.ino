
#define RXD2 16
#define TXD2 17
#define CHARSEXPECTED 43


#define FLAG  0x7E      //tilde
#define FRAMEFORMAT 0x80

const int msgLen = 45;
byte telGram[53][256];
byte TG[256];
byte FrameSeq;

int i;
int j;
int k;
int n;

short counter;
byte charCounter[256];

short V[256];
int I[256];
short Frame[256];
float Volt;
float Current;
int sampleNr;

//test
bool zoekTwee;

unsigned long _start;
unsigned long _cycles;
unsigned long _tussenTijd[256];

//DFT 
#define N 52          // samples
#define Ndiv2 26
#define Fs 2600       // sampling freq = 1/50Hz/52samples 

float X_re[26];
float X_im[26];
float Mag_f[26];

/*E-MUCS Frame
 *  Flag          = 1 byte
 *  Frame Format  = 2 bytes
 *  Address Field = 1 byte
 *  Control Field = 1 byte
 *  Data          = 37 bytes
 *  CRC           = 2 bytes
 *  Flag          = 1 byte
 */

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
 *     Voltage L1 = byte 25-26 in 25 mV //? eg: 0x0B = 11 V + 0x15 = 21 x 25mV = 525 mV -> 11.525mV 
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

 void calcDFT(){
  for (k = 0;k < Ndiv2 ; k++){
    for (n = 0;n < N; n++){
      X_re[k] = X_re[k] + V[n]*25/1000*cos(2*PI*k*n/N);
      X_im[k] = X_im[k]+ V[n]*25/1000*sin(2*PI*k*n/N);
    }
    Mag_f[k] = 2 * sqrt(X_re[k]*X_re[k] + X_im[k]*X_im[k])/N; 
  }
  
 }

 //Make Signal-Todo

void setup() {
  // put your setup code here, to run once: 
  Serial.begin(115200);
  Serial1.begin(2000000,SERIAL_8N1, RXD2, TXD2);
  zoekTwee = true;
  counter = 0;

  //Set DFT Coeff to zero;
  for (k=0;k<Ndiv2;k++){
    X_re[k]=0;
    X_im[k]=0;
    Mag_f[k]=0;
  } 
  
  Serial.println("Started:");
  Serial.print("Chars in buffer at Start: ");
  Serial.println(Serial1.available());
  delay(1000);
  
  //empty buffer
  Serial1.readBytes(TG,255);

}

void loop() {
  // put your main code here, to run repeatedly:

  _start = ESP.getCycleCount();
  //Test
  while(counter < 53) {
      zoekTwee = true;
      while(zoekTwee){
        if (Serial1.available() > 42) {
            while (Serial1.read()!=FLAG) {
              
            }
            if (Serial1.read()==FLAG){
              zoekTwee=false;
              if (Serial1.available() > 42) {
                  charCounter[counter]=Serial1.readBytes(telGram[counter],43);
                  charCounter[counter]=Serial1.available();     // test om underflow te vermijden
                  _tussenTijd[counter]=ESP.getCycleCount()-_start;
                  //Frame 1
                          Frame[counter]=telGram[counter][22];
                          V[counter] = (short)(telGram[counter][23]<<8 | telGram[counter][24]);
                          if (telGram[counter][25] & 0x80) {
                            // To convert 3 byte Two
                            I[counter] = (int)(0xFF<<24 | telGram[counter][25]<<16 | telGram[counter][26]<<8 | telGram[counter][27]);
                          }else {
                            I[counter] = (int)(telGram[counter][25]<<16 | telGram[counter][26]<<8 | telGram[counter][27]);
                          }
                  counter = counter + 1;
              };
            };
          }
      }
      
  }
  /*
  for (i=0;i<53;i++) {
    //for (j=0;j<43;j++) {
      Serial.print("Frame[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(Frame[i],DEC);
      
    //}
  }
  */
  
  for (i=0;i<53;i++) {
      /*
      Serial.print("V[");
      Serial.print(i);
      Serial.print("]: ");
      */
      Volt = 25 * (float)V[i]/1000;
      Serial.println(Volt,2);
      //Serial.println(telGram[i][23],HEX);
      //Serial.println(telGram[i][24],HEX);
      
      Serial.print("I[");
      Serial.print(i);
      Serial.print("]: ");
      Current = I[i]/2;
      Serial.println(Current,2);
      
      //Serial.println(I[i],BIN);
      //Serial.println(telGram[i][25],HEX);
      //Serial.println(telGram[i][26],HEX);
      //Serial.println(telGram[i][27],HEX);
  }

  //DFT  see http://practicalcryptography.com/miscellaneous/machine-learning/intuitive-guide-discrete-fourier-transform/
  calcDFT();
  
  // DFT coeff
  /*
  for (k=0;k<Ndiv2;k++){
    Serial.print("X_re[");
    Serial.print(k);
    Serial.print("]: ");
    Serial.print(k*Fs/N);
    Serial.print(" Hz: ");
    Serial.println(X_re[k]);
    Serial.print("X_im[");
    Serial.print(k);
    Serial.print("]: ");
    Serial.print(k*Fs/N);
    Serial.print(" Hz: ");
    Serial.println(X_im[k]);
  }
  */
  // Magnitude
  
  for (k=0;k<Ndiv2;k++){
    Serial.print("Mag_f[");
    Serial.print(k);
    Serial.print("]: ");
    Serial.println(Mag_f[k],2);
  }
  

  delay(5000);

  //hernieuw meting
  zoekTwee = true;
  counter = 0;
  //empty buffer
  Serial1.readBytes(TG,255);

  
  for (k=0;k<Ndiv2;k++){
    X_re[k]=0;
    X_im[k]=0;
    Mag_f[k]=0;
  }
    


}
