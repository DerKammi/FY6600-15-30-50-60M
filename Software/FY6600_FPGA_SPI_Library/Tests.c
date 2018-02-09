/**
  ******************************************************************************
  * @file    Tests.c 
  * @author  Fremen67
  * @version 
  * @date    03-February-2018
  * @brief   Test Module
  ******************************************************************************
  * @attention
  *
  *
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "LIB_Config.h"
#include "FPGA.h"
#include "Tests.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint32_t CH1_Samples[100];
uint32_t CH2_Samples[100];

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

void Test_WriteFlashRampWave(uint32_t FlashAddr) 
{
  uint32_t i;
	
  FPGA_EraseFlashWave(FlashAddr);
	
  FPGA_WriteCdeReg(0x12,FlashAddr);
  FPGA_WriteCdeReg(0x17,0x01);
  FPGA_WriteCdeReg(0x17,0x00);
	
  // Create Ramp for test purposes
   for(i=0; i<=0x1FFF; i++)
  {
    FPGA_WriteFlashWord(i<<1);
  }
}  

void Test_WriteAndResampleWave(uint32_t FlashAddr)
{
  Test_WriteFlashRampWave(0x1F8000);
  FPGA_ResampleWave(0x1F8000, CH1_Samples);
}

/**
  * @brief  Test of a standard service telegram
  *         
  * @param  None
  *         
  * @retval None
**/
void Test_ServiceTelegram(void)
{
  FPGA_WriteCdeReg(0x1D,0x02);
  FPGA_WriteCdeReg(0x24,0x00);
	  
  FPGA_WriteCdeReg(0x2B,0x7FF);
  FPGA_WriteCdeReg(0x2C,0x7FF);
	
  FPGA_WriteCdeReg(0x06,0x80);
  FPGA_WriteCdeReg(0x06,0x80);

//  FPGA_WriteCdeReg(0x2D,0xE65); // 5.00V
  FPGA_WriteCdeReg(0x2D,0x150); // 2.50V
  FPGA_WriteCdeReg(0x2E,0x150);	

//  FPGA_WriteCdeReg(0x02,0x0D40);  // 20Khz
  FPGA_WriteCdeReg(0x02,0x5F5E100);  
  FPGA_WriteCdeReg(0x01,0x00);	
  FPGA_WriteCdeReg(0x04,0x5F5E100);
  FPGA_WriteCdeReg(0x03,0x00);	

  FPGA_WriteCdeReg(0x2F,0x10000);
  FPGA_WriteCdeReg(0x30,0x10000);	

  FPGA_WriteCdeReg(0x14,0x00);
  FPGA_WriteCdeReg(0x16,0x00);
  while(FPGA_ReadReg(0x18));
		
  // Load Sine in FPGA for CH1
  FPGA_UpdateFpgaWaveFromFlash(1, 0x1FC000);

  // Load Sine in FPGA for CH2
  FPGA_UpdateFpgaWaveFromFlash(2, 0x60000);

  FPGA_WriteCdeReg(0x05,0x00);
  FPGA_WriteCdeReg(0x38,0x7FFDFFF);
  FPGA_WriteCdeReg(0x39,0x7FFDFFF);
	
  FPGA_WriteCdeReg(0x08,0x0FFFFF);
  FPGA_WriteCdeReg(0x09,0x0FFFFF);
  FPGA_WriteCdeReg(0x37,0x01);
  FPGA_WriteCdeReg(0x37,0x00);
	
}

/**
  * @brief  Startup sequence of FPGA
  *         
  * @param  None
  *         
  * @retval None
**/
void Test_Startup(void)
{
  FPGA_Startup();
	
  // FPGA Resample CH1 & CH2 for Display
  FPGA_ResampleWave(0x60000, CH1_Samples);
  FPGA_ResampleWave(0x1FC000, CH2_Samples);
	
  // Load Sine in FPGA for CH1
  FPGA_UpdateFpgaWaveFromFlash(1, 0x60000);

  // Load Arb64 in FPGA for CH2
  FPGA_UpdateFpgaWaveFromFlash(2, 0x1FC000);
	
}

/*-------------------------------END OF FILE-------------------------------*/

