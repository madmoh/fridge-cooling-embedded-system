//===============================================================================
// LCD Routines
//===============================================================================

typedef struct _scrollData
{
   word LCDstartDelay;
   word LCDcharDelay;
   word LCDdelayCounter;
   word LCDlength;
   byte LCDstate;
   char* LCDscrollData;
   int LCDindex;
} ScrollData;

//static volatile byte lcdCnt;    // Controlled by the RTI
#ifdef miniDragon
#define LCD_ENABLE     4        // Strobe data into the LCD module
#define LCD_WRITE_DATA 8        // Select command/data
#else
#define LCD_ENABLE     2        // Strobe data into the LCD module
#define LCD_WRITE_DATA 1        // Select command/data
#endif

    
// Data is sent 4-bits high nibble first.
#define WRITE_CONTROL(c)\
  LCDWriteNibble(c);  c <<= 4; LCDWriteNibble(c);

#define LCD_SET_ADDRESS   0x80 // Write address command
#define LCD_CLEAR         0x10 // Write address command
#define SPACE_CHAR        0x20
// Adjust delay timers to set scroll rates.
// These are default, use functions to set.
#ifdef miniDragon
#define LCD_START_COUNTS    0x2000
#define LCD_CHAR_COUNTS     0x900
#else
#define LCD_START_COUNTS    0x5000
#define LCD_CHAR_COUNTS     0x2000
#endif

#define LCD_WIDTH           20
#define NO_SCROLL           255
#define LCD_WRITE_DELAY     250


void LCD_Init(void);
void LCDUpdateScroll(void);
void LCDWriteLine(byte line, char* d);
void LCDWriteInt( int num);
void LCDWriteFloat( float num);
void LCD_clear_line(int line); 
void LCD_clear_disp(void);
void LCDScrollLine(byte line, char* d );
ScrollData* LCDSetStartDelay( int which, word delay);
ScrollData* LCDSetCharDelay( int which, word delay);
void delay(byte ms);
