#include <arduino.h>
#include <SPI.h>
#include "Lcd12864.h"

Lcd12864::Lcd12864(byte pBgLight, byte pMOSI, byte pCLK, byte pAO, byte pREST, byte pCS)
{
	pinBGLight = pBgLight;
	pinMOSI = pMOSI;
	pinCLK = pCLK;
	pinAO = pAO;
	pinREST = pREST;
	pinCS = pCS;
}

void Lcd12864::sendByte(uint8_t Dbyte)
{
  digitalWrite(pinCS, LOW);
#if PHYSICAL_SPI
  SPI.transfer(Dbyte);
#else
  uint8_t TEMP; 
  TEMP=Dbyte;
  for(int i = 0; i < 8; i++)
  {
    digitalWrite(pinCLK, LOW);
    TEMP= (Dbyte << i) & 0X80;
    digitalWrite(pinMOSI, TEMP);
    digitalWrite(pinCLK, HIGH);
  }
#endif
  digitalWrite(pinCS, HIGH);
}

void Lcd12864::sendCmd(uint8_t command)
{
  digitalWrite(pinAO, LOW);
  sendByte(command);
}

void Lcd12864::sendData(uint8_t Dbyte)
{
  digitalWrite(pinAO, HIGH);
  sendByte(Dbyte);
}

void Lcd12864::switchBgLight(boolean value) {
	digitalWrite(pinBGLight, !value);
}

void Lcd12864::reset() {
    digitalWrite(pinCS, LOW);
    digitalWrite(pinREST, LOW);  
    delay(200);
    digitalWrite(pinREST, HIGH);
    delay(1000);	
    
	sendCmd(0xE2); //system reset
    delay(200);
}

void Lcd12864::setup()
{
  pinMode(pinBGLight, OUTPUT);
  pinMode(pinMOSI, OUTPUT);  
  pinMode(pinCLK, OUTPUT);
  pinMode(pinAO, OUTPUT);
  pinMode(pinREST, OUTPUT);  
  pinMode(pinCS, OUTPUT);  

#if PHYSICAL_SPI
  // initialize SPI:
  SPI.begin(); 
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
#endif	

  reset();
  
  sendCmd(0x24);//SET VLCD RESISTOR RATIO
  sendCmd(0xa2);//BR=1/9
  sendCmd(0xa0);//set seg direction
  sendCmd(0xc8);//set com direction
  sendCmd(0x2f);//set power control
  sendCmd(0x40);//set scroll line
  sendCmd(0x81);//SET ELECTRONIC VOLUME
  sendCmd(0x20);//set pm: 通过改变这里的数值来改变电压 Change the voltage by changing the value here
  //sendCmd(0xa6);//set inverse display	   a6 off, a7 on
  //sendCmd(0xa4);//set all pixel on
  sendCmd(0xaf);//set display enable

  clear();
}

/*************************
 * 取模顺序是列行式，
 * 从上到下，高位在前，从左到右；
 * 先选择页地址0-7，再选择列0-130
 * 页码是直接读取8位数据作为地址；
 * 列是先读取高四位，后读取低四位；
 *
 * The order of sampling is column-lined,
 * From top to bottom, the high position is in front, from left to right;
 * First select page address 0-7, then select column 0-127
 * The page number is to directly read 8-bit data as an address;
 * The column reads the upper four bits first, then reads the lower four bits;
 **********************/


void setPosition(uint8_t command, uint8_t row,uint8_t col1, uint8_t col2){
  sendCmd(command+row);
  sendCmd(0x10+col1);
  sendCmd(0x00+col2);
}


void setCharPosition(uint8_t command, uint8_t row,uint8_t col){
  setPosition(command, row, (8*col/16), (8*col%16))
}

uint16_t writeRow(uint16_t X, uint8_t length, uint8_t command, uint8_t row,uint8_t col, uint8_t const *put){
    setCharPosition(command, row, col);
    for(uint16_t i=0;i<length;i++) {
      sendData(put[X++]);
    }
}

/*****************
 * 8*16字符
 * 8*16 characters
 **********************/
void Lcd12864::render8x8(uint8_t row,uint8_t col,uint8_t count,uint8_t const *put)
{		
  uint16_t X=0;
  for(uint16_t j=0;j<count;j++){
    X = writeRow(X, 8, 0xb0, row, col, put);
  }
}

/*****************
 * 8*16字符
 * 8*16 characters
 **********************/
void Lcd12864::render8x16(uint16_t X, uint8_t row,uint8_t col,uint8_t count,uint8_t const *put)
{		
  uint16_t X=0;
  for(uint16_t j=0;j<count;j++)
  {
    X = writeRow(X, 8, 0xb0, row, col, put);
    X = writeRow(X, 8, 0xb1, row, col, put);
    col+=1;
  } 
}

/*****************
 * 16*16字符
 * 16*16 characters
 **********************/
void Lcd12864::render16x16(uint8_t row,uint8_t col,uint8_t count,uint8_t const *put)
{		
  uint16_t X=0;
  for(uint16_t j=0;j<count;j++)
  {
    X = writeRow(X, 16, 0xb0, row, col, put);
    X = writeRow(X, 16, 0xb1, row, col, put);
    col+=2;
  }
}

/*****************
 * 24*24字符
 * 24*24 characters
 **********************/
void Lcd12864::render24x24(uint8_t row,uint8_t col,uint8_t count,uint8_t const *put)
{		
  uint16_t X=0;
  for(uint16_t j=0;j<count;j++)
  {
    X = writeRow(X, 24, 0xb1, row, col, put);
    X = writeRow(X, 24, 0xb2, row, col, put);
    X = writeRow(X, 24, 0xb0, row, col, put);
    col+=3;
  }
}

/*****************
 * 图片；
 * Renders Picture;
 **********************/
void Lcd12864::renderBmp(uint8_t const *put)
{		
  uint16_t X=0;
  for(uint16_t j=0;j<8;j++)
  {
    setPosition(0xb0, j, 0, 0)
    for(uint16_t i=0;i<128;i++) sendData(put[X++]);
  }	
}

/*****************
 * 图片反显；
 * Renders Picture Inverted
 **********************/
void Lcd12864::renderReversedBmp(uint8_t const *put)
{
  uint16_t X=0;
  for(uint16_t j=0;j<8;j++)
  {
    setPosition(0xb0, j, 0, 0)
    for(uint16_t i=0;i<128;i++) sendData(~put[X++]);
  }	
}

/*****************
 * 清屏；
 * Clear screen;
 **********************/
void Lcd12864::clear()
{	 
  for(uint8_t j=0;j<8;j++)
  {    
    setPosition(0xb0, j, 0, 0)
    for(uint8_t x=0;x<128;x++)  sendData(0);
  }	
}	 
