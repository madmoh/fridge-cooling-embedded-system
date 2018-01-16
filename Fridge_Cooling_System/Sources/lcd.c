//===============================================================================
// Dragon12/miniDragon+ - LCD Routines
// Connected to portM (PTM)
//  Initialization:
// - A list of control condes must be sent.
//
//  The FIRST Line on the LCD is ** 1 ** not ** 0 **
//  miniDragon:
//  LCD routines use 4-bit transfer via port M
//  PM3 ------- RS ( register select, 0 = register transfer, 1 = data transfer).
//  PM2 ------- Enable ( write pulse )
//  PM4 ------- Data Bit 4 of LCD
//  PM5 ------- Data Bit 5 of LCD
//  PM6 ------- Data Bit 6 of LCD
//  PM7 ------- Data Bit 7 of LCD
//
//===============================================================================
#include "derivative.h"
#include "lcd.h"
#include <stdio.h>

//===============================================================================
// This is a list of init commands that are sent to the LCD.
// We only have 4 bits - so for the first 4 commands we only send a nibble.
// Then we send two nibble, hi byte first.
//===============================================================================
byte _lcd_init[]={
  0x30,	// 1st reset code, must delay 4.1ms after sending
  0x30,	// 2nd reset code, must delay 100us after sending
 // all following 10 nibbles must be delay 40us each after sending
  0x30, // 3rd reset code,
  0x20,	// 4th reset code - we are now in 4 bit mode - start 2 pass transfers.
  0x20,	// 4 bit mode, 2 line, 5X7 dot
  0x80,	// 4 bit mode, 2 line, 5X7 dot
  0x00, // cursor increment, disable display shift
  0x60,	// cursor increment, disable display shift
  0x00,	// display on, cursor off, no blinking
  0xC0,	// display on, cursor off, no blinking
  0x00,	// clear display memory, set cursor to home pos
  0x10,	// clear display memory, set cursor to home pos
  0x80,  // set line1
  0x00,  // Set line1
};

int _strlen(char *s)
{
    int i=0;
    while(*(s++) )
        i++;
    return i;
}
char* _strcat(char* s1, char* s2)
{
  char *s = s1;
  while(*s1 )   // move to end of s1
     s1++;
  while(*(s2) ) // copy s2 into s1
    *(s1++) = *(s2++) ;
   *s1 = 0;
   return s;
} 

// The length of a string not counting trailing spaces.
int _trimlen(char * s)
{
   int i = _strlen(s) - 1;
   s += i;
   while(*(s--) == 0x20 )
     i--;
   return i;
}
//===============================================================================
// Delay
//===============================================================================
void delay(byte ms)
{   int i,j;
for(i=0;i<(ms*100);i++)for(j=0;j<14;j++) asm("nop\n");


}
//===============================================================================
// Send the upper nibble of the byte passed in  to LCD.
//===============================================================================
void LCDWriteNibble(byte n)  
{
  n &= 0xf0;

#ifdef miniDragon
  PTM &= 0xf;             // clear upper nibble of port.
  PTM |= n;               // or in the data byte
  delay(1);               // let it settle
  PTM |= LCD_ENABLE;      // Raise the strobe line.
  PTM &= ~LCD_ENABLE;     // lower the strobe line.
#else
  n >>= 2;
  n &= 0x3c;
  PORTK &= ~0x3c;         // zero out the bits
  PORTK |= n;             // set the bits.
  PORTK |= LCD_ENABLE;    // Strobe the data in
  PORTK &= ~LCD_ENABLE;
#endif  
  delay(1);    
}

//===============================================================================
// Write 2 nibbles  to LCD.
//===============================================================================
void LCDWriteChar( byte d ) 
{
#ifdef miniDragon  
  PTM |= LCD_WRITE_DATA;   // Set the command data line to data.
#else
  PORTK |=  LCD_WRITE_DATA;
#endif  

  LCDWriteNibble(d);      // Write the upper nibble.
  d <<= 4;                // Shift the lower nibble up to the upper.
  LCDWriteNibble(d);      // Write the upper nibble.
}
//===============================================================================
// Initalize the LCD Controller
//===============================================================================
void LCD_Init() 
{
  byte i = 0;
  
  
#ifdef miniDragon
  DDRM = 0xff;          // LCD set portm pins to output
  PTM = 0;
#else
  DDRK =  0x3f;         // PK[0:5] Output
  PORTK = 0;
#endif

  delay(100);            // Wait 15ms for first command.
  
  for( ; i < sizeof(_lcd_init) ; ++i ) 
  {
    LCDWriteNibble(_lcd_init[i]);
    delay(50);
  }
  
}
//===============================================================================
// These commands set the LCD controller to a given line.
// From the spec.
//===============================================================================
byte _line_control[]={
  0x00,   // Line 1
  0x40,   // Line 2
  0x14,   // Line 3
  0x54    // Line 4
};
//===============================================================================
//  LCDWriteLine - Which line 1-4 and a string.
//===============================================================================
void LCDWriteLine(byte line, char* d) 
{
  byte c = 0;
#ifdef miniDragon  
  PTM = 0;
#else
  PORTK &= ~LCD_WRITE_DATA;
#endif  
  c = _line_control[line-1] | LCD_SET_ADDRESS;
  
  WRITE_CONTROL(c);
  
  c = 0;
  while( *d && (c < LCD_WIDTH) )
  {
    if(*d == '\r' )
      break;
    LCDWriteChar(*(d++));
    c++;
  }

  // Clear to the end of the line....
//  while( c++ < LCD_WIDTH )
 //   LCDWriteChar(SPACE_CHAR);
    
}

void LCDWriteInt(int num) {
  char Voutbuf[50];   /*Creates a char Voutbuffer */
  byte c=0;
  char *d;
  sprintf(Voutbuf,"%d",num);
  d=Voutbuf;
  PORTK &= ~LCD_WRITE_DATA;
      
  while( *d && (c < LCD_WIDTH) )
  {
    if(*d == '\r' )
      break;
    LCDWriteChar(*(d++));
    c++;
  }

  
}

void LCDWriteFloat(float num) {
  char Voutbuf[50];   /*Creates a char Voutbuffer */
  byte c=0;
  char *d;
  sprintf(Voutbuf,"%4.4f",num);
  d=Voutbuf;
  PORTK &= ~LCD_WRITE_DATA;
      
  while( *d && (c < LCD_WIDTH) )
  {
    if(*d == '\r' )
      break;
    LCDWriteChar(*(d++));
    c++;
  }

}

void LCD_clear_line(line) {
LCDWriteLine(line,"                ");
}

void LCD_clear_disp() {
LCDWriteLine(1,"                ");
LCDWriteLine(2,"                ");
}
//===============================================================================
//===============================================================================

ScrollData _sd[]=
{
{LCD_START_COUNTS,LCD_CHAR_COUNTS,0,LCD_WIDTH,-1,(char*)0,0},
{LCD_START_COUNTS,LCD_CHAR_COUNTS,0,LCD_WIDTH,-1,(char*)0,0},

//#ifdef LDC_4LINES
{LCD_START_COUNTS,LCD_CHAR_COUNTS,0,LCD_WIDTH,-1,(char*)0,0},
{LCD_START_COUNTS,LCD_CHAR_COUNTS,0,LCD_WIDTH,-1,(char*)0,0},
//#endif

{0}}; // IN CW this generates a warning which is just plain wrong!


//===============================================================================
// External access functions.
// which - line # 1-4
//===============================================================================
ScrollData* LCDSetStartDelay( int which, word delay)
{
   _sd[which-1].LCDstartDelay = delay;
   return &_sd[which-1];
}
ScrollData* LCDSetCharDelay( int which, word delay)
{
   _sd[which-1].LCDcharDelay = delay;
   return &_sd[which-1];
}

//===============================================================================
// Write a line to the LCD to be scrolled.
// if d  = 0 then stop scolling that line.
//===============================================================================
void LCDScrollLine( byte line, char* d )
{
  if( 0 == d )  // No more scrolling please.
  {
    _sd[line-1].LCDstate = NO_SCROLL;
    return;
  }

  LCDWriteLine(line, d );

  // Init the scoll timers.
  _sd[line-1].LCDindex = 0;
  _sd[line-1].LCDdelayCounter =  0;
  _sd[line-1].LCDscrollData = d;

  // If it is less than the width, then don't scoll.
  if( _trimlen(d) <  LCD_WIDTH )
     _sd[line-1].LCDstate = NO_SCROLL;
  else
     _sd[line-1].LCDstate = 0;
}

//===============================================================================
// LCDUpdateScroll MUST be called if scrolling is to work.
// The time constants depend on how often it is called.
//===============================================================================
void _LCDUpdateScroll(byte line);
void LCDUpdateScroll()
{
  byte i;

  // For each line update the scoll states.
  for( i = 0 ;  ; ++i)
  {
     if( 0 == _sd[i].LCDstartDelay )
       break;

     _LCDUpdateScroll(i);
  }
}
void _LCDUpdateScroll(byte index)
{
  if( NO_SCROLL ==  _sd[index].LCDstate )  // Means no scrolling.
    return;

  if( _sd[index].LCDstate == 0 )  // First write delay.
  {
     if(_sd[index].LCDdelayCounter >= _sd[index].LCDstartDelay)
     {
        _sd[index].LCDstate = 1;
        _sd[index].LCDindex = 1;
        _sd[index].LCDdelayCounter =  0;
     }
  }
  else if( _sd[index].LCDstate == 1 ) // Each character delay.
  {

     if( _sd[index].LCDdelayCounter >= _sd[index].LCDcharDelay )
     {
       if(_sd[index].LCDscrollData[_sd[index].LCDindex] == 0 )
       {
          // hit the end, start over.
          _sd[index].LCDindex = 0;
          _sd[index].LCDstate = 0;
       }
       else
       { // Move to the next char.
         ++_sd[index].LCDindex;
       }
       // Update the lline on the LCD, shifted.
       LCDWriteLine(index+1, &_sd[index].LCDscrollData[_sd[index].LCDindex]);
       _sd[index].LCDdelayCounter =0;
     }
  }

  ++_sd[index].LCDdelayCounter;
}
