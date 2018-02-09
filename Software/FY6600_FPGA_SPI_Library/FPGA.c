/**
  ******************************************************************************
  * @file    FPGA.c 
  * @author  Fremen67
  * @version V0.1
  * @date    03-February-2018
  * @brief   Basic communication functions with Feeltech FY6600 FPGA
  ******************************************************************************
  * @attention
  *
  * Retries and/or Time-out and error handling still to be added
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "LIB_Config.h"
#include "FPGA.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Wait for FPGA to be ready.
  * @param  None 
  * @retval None
  */
static void FPGA_WaitReady() 
{
  uint16_t TimeON = 0;
  uint16_t TimeOFF = 0;
	
  // Wait for 100ms continuous ON State of FPGA_Ready signal: FPGA code loading launched
  while (TimeON <100)
  {		
    TimeON = __FPGA_READY_STATE() ? (++TimeON):0;
    delay_us(1000);
  }
	
  // Wait for 5 ms continuous OFF State of FPGA_Ready signal: FPGA loaded and ready
  while (TimeOFF < 5)
  {		
    TimeOFF = __FPGA_READY_STATE() ? 0:(++TimeOFF);
    delay_us(1000);
  }
}

/**
  * @brief  Write one command and its parameter in the FPGA Control Register
  * @param  Reg: Specifies the register to be written 
  * @param  Param: Specifies the value to write in the register
  * @retval None
  */
void FPGA_WriteCdeReg(uint16_t Reg, uint32_t Param) 
{
  __FPGA_DC_CLR();
  __FPGA_CS_CLR();
  __FPGA_WRITE_WORD(Reg);
  __FPGA_DC_SET();
	
  __FPGA_WRITE_WORD(Param>>16);
  __FPGA_WRITE_WORD(Param);

  __FPGA_CS_SET();
}  

/**
  * @brief  Ask for a register value through the FPGA Control Register
  * @param  Reg: Specifies the register which is requested 
  * @retval None
  */
static void FPGA_RequestReg(uint16_t Reg) 
{
  __FPGA_DC_CLR();
  __FPGA_CS_CLR();
  __FPGA_WRITE_WORD(Reg);
  __FPGA_CS_SET();
  __FPGA_DC_SET();
}  

/**
  * @brief  Get the value requested with FPGA_RequestReg
  * @param  None 
  * @retval Register value
  */
static uint32_t FPGA_GetRequestedReg() 
{
  uint32_t RetValueMSW;
  uint32_t RetValueLSW;
	
  __FPGA_CS_CLR();
  RetValueMSW = __FPGA_WRITE_WORD(0x00);
  RetValueLSW = __FPGA_WRITE_WORD(0x00);
  __FPGA_CS_SET();
	
  return (RetValueMSW << 16) | RetValueLSW;
}	

/**
  * @brief  Get a register value through the FPGA Control Register
  * @param  Reg: Specifies the wanted register
  * @retval Register value
  */
uint32_t FPGA_ReadReg(uint16_t Reg) 
{
  FPGA_RequestReg(Reg);
  return FPGA_GetRequestedReg();
}  

/**
  * @brief  Update FPGA waveform buffer with a specific waveform from flash memory
  * @param  Channel: Specifies which buffer to update
  *   This parameter can be one of following parameters:
  *     @arg 0: for test purpose
  *     @arg 1: Channel 1
  *     @arg 2: Channel 2
  * @retval None
  */
void FPGA_UpdateFpgaWaveFromFlash(uint32_t Channel, uint32_t FlashStartAddr) 
{
  FPGA_WriteCdeReg(0x14,0x00);          // Clear flash write trigger
  FPGA_WriteCdeReg(0x16,0x00);          // Clear flash page erase trigger

  while(FPGA_ReadReg(0x18));            // Go on when no pending flash operation

  FPGA_WriteCdeReg(0x12,FlashStartAddr);// Waveform start address in Flash
  FPGA_WriteCdeReg(0x11,Channel);       // FPGA Channel to be updated
  FPGA_WriteCdeReg(0x13,0x01);          // Trigger flash read operation
  FPGA_WriteCdeReg(0x13,0x00);
  FPGA_WriteCdeReg(0x11,0x00);	
	
  FPGA_RequestReg(0x18);                // Wait for end of operation
  while (FPGA_GetRequestedReg());
}  

/**
  * @brief  Read one byte from flash memory through FPGA
  * @param  ReadAddr: Specifies the address to read
  * @retval byte read from flash
  */
static uint32_t FPGA_ReadFlashByte(uint32_t ReadAddr) 
{
  FPGA_WriteCdeReg(0x12,ReadAddr);     // Byte address in Flash memory
  FPGA_WriteCdeReg(0x0F,0x01);         // Trigger read operation
  FPGA_WriteCdeReg(0x0F,0x00);
	
  FPGA_RequestReg(0x18);
  while (FPGA_GetRequestedReg());      // Wait for end of read operation
	
  FPGA_RequestReg(0x0E);               // Get read value
  return FPGA_GetRequestedReg();  			
}  

/**
  * @brief  Read one word from flash memory through FPGA
  * @param  ReadAddr: Specifies the start address to read
	* @retval Word read from flash
  */
static uint32_t FPGA_ReadFlashWord(uint32_t ReadAddr) 
{
  return FPGA_ReadFlashByte(ReadAddr)|(FPGA_ReadFlashByte(ReadAddr+1)<<8);  			
}  

/**
  * @brief  Take 100 samples of a waveform out of 8192 from flash memory through FPGA
  * @param  ReadAddr: Specifies the start address of the Waveform in flash memory to resample
  * @param  Samples: Specifies a pointer to the array which will receive the 100 samples
  * @retval None
  */
void FPGA_ResampleWave(uint32_t ReadAddr, uint32_t *Samples) 
{
  uint32_t i;
	
  FPGA_WriteCdeReg(0x14,0x00);         // Clear flash write trigger
  FPGA_WriteCdeReg(0x16,0x00);         // Clear flash page erase trigger
	
  // Resample 100 points out of 8192
  for(i=0; i<=99; i++)
  {
    Samples[i] = FPGA_ReadFlashWord(ReadAddr + i*164);
  }
}

/**
  * @brief  Erase one page of flash memory through FPGA (4096 bytes)
  * @param  FlashStartAddr: Specifies the start address of the page to erase
  * @retval None
  */
static void FPGA_EraseFlashPage(uint32_t FlashStartAddr) 
{
  FPGA_WriteCdeReg(0x12,FlashStartAddr);
  FPGA_WriteCdeReg(0x16,0x01);         // Trigger erase page operation
  FPGA_WriteCdeReg(0x16,0x00);
  FPGA_RequestReg(0x18);
  while (FPGA_GetRequestedReg());      // Wait for end of operation
}  

/**
  * @brief  Erase one waveform from flash through FPGA (4 pages = 16384 bytes)
  * @param  FlashStartAddr: Specifies the start address of the waveform to erase
* @retval None
  */
void FPGA_EraseFlashWave(uint32_t FlashStartAddr) 
{
  FPGA_WriteCdeReg(0x14,0x00);                   // Clear flash write trigger
  FPGA_WriteCdeReg(0x16,0x00);                   // Clear flash page erase trigger
  FPGA_RequestReg(0x18);
  while (FPGA_GetRequestedReg());                // Wait for no pending flash operation
	
  FPGA_EraseFlashPage(FlashStartAddr);			     // Erase 4 flash pages (1 page = 4096 bytes)
  FPGA_EraseFlashPage(FlashStartAddr + 0x1000);
  FPGA_EraseFlashPage(FlashStartAddr + 0x2000);
  FPGA_EraseFlashPage(FlashStartAddr + 0x3000);
}

/**
  * @brief  Write one byte in Flash memory through FPGA (Address already defined by Reg0x12 and Reg0x17 trigger)
  * @param  Byte: Value to write
  * @retval None
  */
static void FPGA_WriteFlashByte(uint32_t Byte) 
{
  FPGA_WriteCdeReg(0x15,Byte&0xFF);
  FPGA_WriteCdeReg(0x14,0x01);
  FPGA_WriteCdeReg(0x14,0x00);
  FPGA_RequestReg(0x18);
  while (FPGA_GetRequestedReg());      // Wait for end of write operation
}  

/**
  * @brief  Write one word in Flash memory through FPGA (Address already defined by Reg0x12 and Reg0x17 trigger)
  * @param  Byte: Value to write
  * @retval None
  */
void FPGA_WriteFlashWord(uint32_t Word) 
{
  FPGA_WriteFlashByte(Word);
  FPGA_WriteFlashByte(Word>>8);
}    
//  ==== Example for using WriteFlashWord =====
//void Test_WriteFlashRampWave(uint32_t FlashAddr) 
//{
//	uint32_t i;
//	
//	FPGA_EraseFlashWave(FlashAddr);
//	
//	FPGA_WriteCdeReg(0x12,FlashAddr);
//	FPGA_WriteCdeReg(0x17,0x01);		// Store start Address
//	FPGA_WriteCdeReg(0x17,0x00);
//	
//	// Create Ramp for test purposes
//  for(i=0; i<=0x1FFF; i++)
//	{
//		FPGA_WriteFlashWord(i<<1);
//	}
//} 

/**
  * @brief  Test flash memory on startup
  * @param  None
  * @retval 0 = Flash test Failure ; 1 = Flash test OK
  */
static uint8_t FPGA_TestFlash() 
{
  // Test FPGA waveform buffer update from flash. Channel = 0 for test
  FPGA_UpdateFpgaWaveFromFlash(0,0xB0000); 
  FPGA_UpdateFpgaWaveFromFlash(0,0xB4000); 
  FPGA_UpdateFpgaWaveFromFlash(0,0xB8000); 
  FPGA_UpdateFpgaWaveFromFlash(0,0xBC000);
	
  // Test reading @0xF0000 (should return 0) and reading @0xF0004 (should return 4)
  return (FPGA_ReadFlashByte(0x0F0000) == 0) && (FPGA_ReadFlashByte(0x0F0004) == 4);
} 

/**
  * @brief  FPGA initialization routine
  * @param  None
  * @retval None
  */
void FPGA_Init() 
{
  uint8_t i;
	
  // Initialize internal FPGA registers
  FPGA_WriteCdeReg(0x25,0x2FAF080);
  FPGA_WriteCdeReg(0x26,0x1DCD6500);
  FPGA_WriteCdeReg(0x28,0x2A05F200);
  FPGA_WriteCdeReg(0x27,0x01);
  FPGA_WriteCdeReg(0x2A,0xA931A000);
  FPGA_WriteCdeReg(0x29,0xE35F);
	
  FPGA_WriteCdeReg(0x24,0x10);         // Modulation Mode init
  FPGA_WriteCdeReg(0x06,0xC5);         // Relays Command init : CH1 & CH2 on low range

  // Purpose still to be found
  for(i=0; i<=15; i++)
  {
    FPGA_WriteCdeReg(0x10,i);
    FPGA_ReadReg(0x1A);
  }
 
  // Purpose still to be found
  for(i=0; i<=15; i++)
  {
    FPGA_WriteCdeReg(0x1B,i);
    FPGA_WriteCdeReg(0x19,1);
    FPGA_WriteCdeReg(0x19,0);
  }
}  

/**
  * @brief  Startup sequence of FPGA    
  * @param  None
  * @retval None
**/
void FPGA_Startup(void)
{
  FPGA_WaitReady();
  FPGA_TestFlash();
  FPGA_Init();	
}

/*-------------------------------END OF FILE-------------------------------*/

