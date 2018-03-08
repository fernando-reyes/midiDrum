#include <math.h>

#define NPADS        16
#define PCUTOFF       6
#define HCUTOFF       100
#define HMCUTOFF      700
#define HIT_FACTOR    0.4//0.36//0.1789
#define LEDPIN       13
#define DBLPEDALPIN   7
#define GENMIDIPIN    8
#define MIDICHANNEL   9
#define KICKPEDAL     1
//#define MAXREAD       5

//Addictive Drums Keymap
unsigned char PadNote[NPADS] = { 7,36,38,71,69,67,65,77,//46,
                                79,81,45,62,47,40,0,0};
//unsigned char PadNote[NPADS] = { 1, 2, 3, 4, 5, 6, 7, 8,
//                                 9,10,11,12,13,14,15,16};
                           
int hits[NPADS];
//int hits_[NPADS];

int pin         = -1;
int hit         =  0;
int hihatSegment=  8;
int hhit        =  0; 
int hlastPos;

bool generalMidi,doublePedal;

int muxTable[16][4] = {{LOW ,LOW ,LOW ,LOW},
                       {HIGH,LOW ,LOW ,LOW},
                       {LOW ,HIGH,LOW ,LOW},
                       {HIGH,HIGH,LOW ,LOW},
                       {LOW ,LOW ,HIGH,LOW},
                       {HIGH,LOW ,HIGH,LOW},
                       {LOW ,HIGH,HIGH,LOW},
                       {HIGH,HIGH,HIGH,LOW},
                       {LOW ,LOW ,LOW ,HIGH},
                       {HIGH,LOW ,LOW ,HIGH},
                       {LOW ,HIGH,LOW ,HIGH},
                       {HIGH,HIGH,LOW ,HIGH},
                       {LOW ,LOW ,HIGH,HIGH},
                       {HIGH,LOW ,HIGH,HIGH},
                       {LOW ,HIGH,HIGH,HIGH},
                       {HIGH,HIGH,HIGH,HIGH}};

int ledCicles = -1;

void setup() {
    
  doublePedal = isPinUp(DBLPEDALPIN);
  
  //GENERAL MIDI
  if( generalMidi = isPinUp(GENMIDIPIN) ){
      
      PadNote[0] = 42; //hh
      PadNote[1] = 35; //kk

      PadNote[3] = 50; //t4
      PadNote[4] = 47; //t3
      PadNote[5] = 43; //t2
      PadNote[6] = 45; //t1
      
      PadNote[7] = 49; //cy
      PadNote[8] = 51; //rd
  }   

  if( doublePedal || generalMidi )
      hihatSegment = 43;  
    
    //encendemos el led de power 
    pinMode(LEDPIN, OUTPUT);
    turnLedOn(-1);
  
    //inicalizamos las variables contenedoras de las lecturas de cada pad
    for( int i=0 ; i<NPADS; i++){
        hits[ i ] = 0;
//        hits_[ i ] = 0;
    }
      
    //abrimos los puertos digitales para el multiplexor
    for( int i = 2 ; i < 7; i++ )
        pinMode(i, OUTPUT);
        
    //inhibit mux
    digitalWrite(6,HIGH);
    
    //hihat switch
    //pinMode(7, INPUT_PULLUP);
    //digitalWrite(7,HIGH);
    pinMode(A2,INPUT_PULLUP);
    //analogWrite(A2,HIGH);
    //baudios MIDI
    Serial.begin(31250);
  //Serial.begin(230400);
}

void loop() {

  if( ++pin == NPADS )  
      pin=0;
      
  //vamos por el mayor    
  while( ( hit = analogRead_( pin ) ) > hits[ pin ] )
  //if( ( hit = analogRead_( pin ) ) > hits[ pin ] )
      hits[ pin ]=hit;
      
  //si hay datos, el resto se desecha (...)
  if( hits[ pin ] && 
     // ( ++hits_[ pin ] > MAXREAD ) &&
     (hit < 10)   &&
     ( pin == KICKPEDAL ? hits[ pin ] > 16 : true ) ) {
      turnLedOn(200); 
      MIDI_TX( 144, PadNote[ pin ] , min( hits[ pin ] * HIT_FACTOR + 1 , 127 ) );
      MIDI_TX( 144, PadNote[ pin ] , 0 );
      hits[ pin ] = 0;
      //hits_[ pin ] = 0;
  }
  
  //HIHAT
  if( ( pin == 0 ) && 
      ( (hhit = analogRead(A2)) >= HCUTOFF ) && ( hhit <= HMCUTOFF ) &&
      ( (hhit = map( hhit , HCUTOFF, HMCUTOFF,127,0 )) >= 0 ) &&
      ( (hhit = (hihatSegment*int(hhit/hihatSegment) ) ) >= 0  ) &&
      ( hhit ==  hihatSegment*int(127/hihatSegment) ? hhit =  127 : 1 ) &&
      ( hhit == 16 ? (hhit=0,1) : 1 ) &&
      ( hhit != hlastPos ) 
      ){
      //hhit = min( hhit/8-1 , 127 );
      hlastPos = hhit;
      
      if( doublePedal ){
          if( hhit == 0 ){
              turnLedOn(200);
              MIDI_TX(144,PadNote[1], 127 );
              MIDI_TX(144,PadNote[1], 0 );
          }
      }else if( generalMidi ){
          if( hhit != hihatSegment ){ 
              if(  hhit == 0 )
                   PadNote[0] = 42;
              else 
                   PadNote[0] = 46;
          }
      }else
          MIDI_TX(176,4, hhit);                
  }

  if ( ledCicles > 0 && --ledCicles == 0 )
      digitalWrite(LEDPIN, LOW);
}

int oldPad=-1;
int analogRead_(int pad){
               
    if( oldPad!=pad ){
        oldPad = pad ;
        digitalWrite(2, muxTable[pad][0] );
        digitalWrite(3, muxTable[pad][1] );
        digitalWrite(4, muxTable[pad][2] );
        digitalWrite(5, muxTable[pad][3] );
    }
    digitalWrite(6, LOW );
    int res = max( analogRead(A0)-PCUTOFF,0 );
    digitalWrite(6, HIGH );
    return res;
}

boolean isPinUp(int digitalPin){
  pinMode(digitalPin, OUTPUT);
  digitalWrite(digitalPin,LOW);
  pinMode(digitalPin,INPUT);
  return digitalRead(digitalPin) != LOW;
}

void turnLedOn(int cicles){
  digitalWrite(LEDPIN, HIGH); 
  ledCicles = cicles;
}

void MIDI_TX(unsigned char CMD, unsigned char NOTE, unsigned char VELOCITY) {
  Serial.write(CMD|MIDICHANNEL);
  Serial.write(NOTE);
  Serial.write(VELOCITY);
}

