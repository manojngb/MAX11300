/* Read and write SPI Slave EEPROM.
 * Tuned for AT93C46 (128 x 8-bits).
 * Windows build instructions:
 *  1. Copy ftd2xx.h and 32-bit ftd2xx.lib from driver package.
 *  2. Build.
 *      MSVC:    cl spim.c LibFT4222.lib ftd2xx.lib
 *      MinGW:  gcc spim.c LibFT4222.lib ftd2xx.lib
 * Linux instructions:
 *  1. Ensure libft4222.so is in the library search path (e.g. /usr/local/lib)
 *  2. gcc spim_adc.c -lft4222 -Wl,-rpath,/usr/local/lib
 *  3. sudo ./a.out
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "ftd2xx.h"
#include "libft4222.h"
#include <unistd.h>///

#ifndef _countof
    #define _countof(a) (sizeof((a))/sizeof(*(a)))
#endif

#define EEPROM_BYTES 128 // AT93C46 has 128 x 8 bits of storage

// SPI Master can assert SS0O in single mode
// SS0O and SS1O in dual mode, and
// SS0O, SS1O, SS2O and SS3O in quad mode.
#define SLAVE_SELECT(x) (1 << (x))

/**static uint8 originalContent[EEPROM_BYTES];
static uint8 newContent[EEPROM_BYTES];
static char slogan1[EEPROM_BYTES + 1] =
    "FTDI Chip strives to Make Design Easy with our modules, cables "
    "and integrated circuits for USB connectivity and display systems.";
static char slogan2[EEPROM_BYTES + 1] =
    "FT4222H: Hi-Speed USB 2.0 QSPI/I2C device controller.  QFN32, "
    "1.8/2.5/3.3V IO, 128 bytes OTP.  Requires 12 MHz external crystal.";

**/


// Reg read function implementation 
int MAX11300Reg_read(FT_HANDLE ftHandle1, uint8 address)
   	{
	FT4222_STATUS ft4222Status;
   	uint8 data[3];
   	data[0] = (address<<1) | 0x01;
   	data[1] = 0;
   	data[2] = 0;
   	uint8 response[3]={0x00 ,0x00 ,0x00};
	uint16 rxdata;
   	uint16 rx2;
	ft4222Status = FT4222_SPIMaster_SingleReadWrite(
                       ftHandle1,
                       response,
                       data,
                       3,
                       &rx2,
                       TRUE);
        if (FT4222_OK != ft4222Status)
        	{
        	printf("FT4222_SPIMaster_SingleReadWrite failed (error %d)!\n",ft4222Status);
		return 0;
        	}
	rxdata = ((response[1])<<8) | response[2]; //Combining into single 
       	printf("Rxdata = 0x%04x\n" , rxdata);
	return 1;		
   	}

// MAX ADC read 12bit
float MAX11300adc_read(FT_HANDLE ftHandle1, uint8 address)
   	{
	FT4222_STATUS ft4222Status;
   	uint8 data[4];
   	data[0] = (address<<1) | 0x01;
   	data[1] = 0;
   	data[2] = 0;
   	uint8 response[3]={0x00 ,0x00 ,0x00};
	uint16 rxdata;
   	uint16 rx2;
	float adc_read;
	ft4222Status = FT4222_SPIMaster_SingleReadWrite(
                       ftHandle1,
                       response,
                       data,
                       3,
                       &rx2,
                       TRUE);
        if (FT4222_OK != ft4222Status)
        	{
        	printf("FT4222_SPIMaster_SingleReadWrite failed (error %d)!\n",ft4222Status);
		return 0;
        	}
	rxdata = ((response[1])<<8) | (response[2]) ; //Combining into single 
	rxdata = rxdata & 0x0FFF ; // extracting 12 bits
       	printf("ADCdata = 0x%04x\n" , rxdata);
	adc_read = ((float)(rxdata)*10/(4095));
	printf("ADCdata = %f \n", adc_read);
	return adc_read ;		
   	}

//Reg write implementation
int MAX11300Reg_write(FT_HANDLE ftHandle1, uint32 command)
	{
	FT4222_STATUS        ft4222Status;
	uint8 data[3];
	data[0] = command>>15 & 0xFE;   // since max address is 0x73 shifting by 15 would not truncate !
	data[1] = command>>8 & 0xFF;	// MSB	
	data[2] = command & 0xFF;	// LSB
//	printf("%x\n", data[0]); printf("%x\n", data[1]); printf("%x\n", data[2]);
   	uint16 rx2;
   	ft4222Status = FT4222_SPIMaster_SingleWrite(
                           ftHandle1,
                           data,
                           3,
                           &rx2,
                           TRUE);
        if (FT4222_OK != ft4222Status)
        	{
        	printf("FT4222_SPIMaster_SingleReadWrite failed (error %d)!\n",ft4222Status);
		return 0;
        	}
        printf("Wrote %06x\n", command);
        sleep(0.002);
	return 1;
   	}

//MAX initialisation and setting up
int MAX11300_init(FT_HANDLE ftHandle1)
	{
	int a;	
	a=MAX11300Reg_write(ftHandle1, 0x1000f0); //Device control register
	a=MAX11300Reg_write(ftHandle1, 0x2A5100); //DAC config for port10
	a=MAX11300Reg_write(ftHandle1, 0x6A0666);	//DAC Data register
	a=MAX11300Reg_write(ftHandle1, 0x207100); //ADC functid,rpm
	a=MAX11300Reg_write(ftHandle1, 0x1000f7); //Device control register- adc continous sweep
	}


int main(void)
{
    FT_STATUS                 ftStatus;
    FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
    DWORD                     numDevs = 0;
    int                       i;
    int                       retCode = 0;
 
    FT_HANDLE            ftHandle = (FT_HANDLE)NULL;
    FT4222_STATUS        ft4222Status;
    FT4222_Version       ft4222Version;
    char                *writeBuffer;

    int MAX11300_status;

    ftStatus = FT_CreateDeviceInfoList(&numDevs);
    if (ftStatus != FT_OK)
    {
        printf("FT_CreateDeviceInfoList failed (error code %d)\n",
               (int)ftStatus);
        retCode = -10;
        goto exit;
    }

    if (numDevs == 0)
    {
        printf("No devices connected.\n");
        retCode = -20;
        goto exit;
    }

    /* Allocate storage */
    devInfo = calloc((size_t)numDevs,
                     sizeof(FT_DEVICE_LIST_INFO_NODE));
    if (devInfo == NULL)
    {
        printf("Allocation failure.\n");
        retCode = -30;
        goto exit;
    }

    /* Populate the list of info nodes */
    ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
    if (ftStatus != FT_OK)
    {
        printf("FT_GetDeviceInfoList failed (error code %d)\n",
               (int)ftStatus);
        retCode = -40;
        goto exit;
    }
    printf("number of devices %d",(int)numDevs);
    for (i = 0; i < (int)numDevs; i++)
    {
        if (devInfo[i].Type == FT_DEVICE_4222H_0)
        	{
            	printf("\nDevice %d is FT4222H in mode 0 (single Master or Slave):\n",i);
            	printf("  0x%08x  %s  %s\n",(unsigned int)devInfo[i].ID, devInfo[i].SerialNumber, devInfo[i].Description);
		//Excercise FT4222 and put in SPI mode
		ftStatus = FT_OpenEx((PVOID)(uintptr_t)devInfo[i].LocId,
                         FT_OPEN_BY_LOCATION,
                         &ftHandle);
    		if (ftStatus != FT_OK)
    			{
        		printf("FT_OpenEx failed (error %d)\n",(int)ftStatus);
        		goto exit;
    			}

    		ft4222Status = FT4222_GetVersion(ftHandle,&ft4222Version);
    		if (FT4222_OK != ft4222Status)
    			{
        		printf("FT4222_GetVersion failed (error %d)\n",(int)ft4222Status);
        		goto exit;
    			}

    		printf("Chip version: %08X, LibFT4222 version: %08X\n",(unsigned int)ft4222Version.chipVersion,(unsigned int)ft4222Version.dllVersion);

    		// Configure the FT4222 as an SPI Master.
    		ft4222Status = FT4222_SPIMaster_Init(
                        ftHandle,
                        SPI_IO_SINGLE, // 1 channel
                        CLK_DIV_32, // 60 MHz / 32 == 1.875 MHz
                        CLK_IDLE_LOW, // clock idles at logic 0
                        CLK_LEADING, // data captured on rising edge
                        SLAVE_SELECT(0)); // Use SS0O for slave-select
    		if (FT4222_OK != ft4222Status)
    			{
        		printf("FT4222_SPIMaster_Init failed (error %d)\n",(int)ft4222Status);
       			goto exit;
    			}
   		ft4222Status = FT4222_SPI_SetDrivingStrength(ftHandle,
                                                 DS_8MA,
                                                 DS_8MA,
                                                 DS_8MA);
    		if (FT4222_OK != ft4222Status)
    			{
        		printf("FT4222_SPI_SetDrivingStrength failed (error %d)\n",(int)ft4222Status);
        		goto exit;
    			}
		printf("=============================================\n");

// Everything works. Time to configure MAX11300
		MAX11300_status = MAX11300_init(ftHandle);
		float adc_read;
		float setpoint;
		uint16 dac_value = 0xF5;
		if (MAX11300_status)
			{
			printf("enter your desired voltage");
			scanf("%f",&setpoint);
			printf("make sure, the series resistor at 0.6V fb doesnt affect circuit. will try my best ;) ");
			while(1)
				{
				// While(1), Locks up at the first device. Use this to read the value of ADC or DAC 
				adc_read = MAX11300adc_read(ftHandle, 0x40);
				if (adc_read > setpoint & dac_value < 0x1EB)
					{
					dac_value = dac_value + 2;
					MAX11300Reg_write(ftHandle, 0x6A0000|dac_value);  //address Plus data
					}
				else if (adc_read < setpoint & dac_value > 51 )
					{
 					dac_value = dac_value - 2;
					MAX11300Reg_write(ftHandle, 0x6A0000|dac_value);
					}
				sleep (.1);
				}
        		}
		}
    }
exit:
    if (ftHandle != (FT_HANDLE)NULL)
    	{
        (void)FT4222_UnInitialize(ftHandle);
        (void)FT_Close(ftHandle);
    	}
    free(devInfo);
}


//------------------------------------------------------------------------------------------------
        /*else
        {
            printf("read size = %d\n" , rx2);
            printf ("Response = 0x%x : 0x%x\n", response[2], response[1]);
        }*/
        ///printf("\n");
   		/*
   		ft4222Status = FT4222_SPIMaster_SingleReadWrite(
                            ftHandle,
                            response,
                            data,
                            3,
                            &rx2,
                            TRUE);
        if (FT4222_OK != ft4222Status)
        {
            printf("FT4222_SPIMaster_SingleReadWrite failed (error %d)!\n",
                   ft4222Status);
            success = 0;
            goto exit;
        }
        else
        {
            printf("read size = %d\n" , rx2);
            printf ("Response = 0x%x : 0x%x\n", response[2], response[1]);
        }
        */

/*uint8 data[3];
   		data[0] = (command<<1) | 0x01;
   		data[1] = 0;
   		data[2] = 0;
		uint16 response;
                uint16 rx2 = 2;
		ft4222Status =FT4222_SPIMaster_SingleRead
				(ftHandle, data, 3, &rx2, TRUE);
        if (FT4222_OK != ft4222Status)
        {
            printf("FT4222_SPIMaster_SingleRead failed (error %d)!\n",
                   ft4222Status);

            success = 0;

            goto exit;

        }

	printf ("Response = 0x%02x\n ", data[0]);*/

        //FILE *f;
        //f = fopen("buffer.txt", "a");
        //fprintf(f, "0x%02x%02x\n", response[1], response[2]);
        //fclose(f);
        //printf("Reading %02x\n", command);
        /*else
        {
   		    printf("0x%x\n", data[0]);
   		    printf("read size = %d\n" , rx2);
            printf ("Response = 0x%2x : 0x%2x\n", response[2], response[1]);
        }
exit:
    if (ftHandle != (FT_HANDLE)NULL)
    	{
        (void)FT4222_UnInitialize(ftHandle);
        (void)FT_Close(ftHandle);
    	}

    return success;
}*/

      //sleep(1e-1);
