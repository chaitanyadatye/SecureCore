/*
 *
 *  Copyright (c) 2014   Warren Jasper <wjasper@tx.ncsu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <asm/types.h>
#include "pmd.h"
#include "usb-1208FS-Plus.h"

#define MAX_COUNT     (0xffff)
#define FALSE 0
#define TRUE 1

/* Test Program */
int toContinue()
{
  int answer;
  answer = 0; //answer = getchar();
  printf("Continue [yY]? ");
  while((answer = getchar()) == '\0' ||
    answer == '\n');
  return ( answer == 'y' || answer == 'Y');
}

int main (int argc, char **argv)
{
  usb_dev_handle *udev = NULL;

  float table_DE_AIN[NGAINS_USB1208FS_PLUS][NCHAN_DE][2];
  float table_SE_AIN[NCHAN_SE][2];

  int ch;
  int i, j, k, m;
  int flag;
  int device;
  __u8 input;
  int temp;
  __u8 options;
  char serial[9];
  __u8 channel, channels;
  __u8 range;
  __u8 ranges[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  __u16 value;
  __u32 count;
  double frequency, voltage;
  int ret;
  __u16 dataAIn[8*512];  // holds 16 bit unsigned analog input data
  __u16 dataAOut[128];   // holds 12 bit unsigned analog input data
  __u16 data;
  int nchan, repeats;

  udev = NULL;
  if ((udev = usb_device_find_USB_MCC(USB1208FS_PLUS_PID))) {
    printf("Success, found a USB 1208FS-Plus!\n");
    device = USB1208FS_PLUS_PID;
  } else if ((udev = usb_device_find_USB_MCC(USB1408FS_PLUS_PID))) {
    printf("Success, found a USB 1408FS-Plus!\n");
    device = USB1408FS_PLUS_PID;
  } else {
    printf("Failure, did not find a USB 1208FS-Plus/ USB 1408FS-Plus!\n");
    return 0;
  }

  // some initialization
  usbBuildGainTable_DE_USB1208FS_Plus(udev, table_DE_AIN);
  usbBuildGainTable_SE_USB1208FS_Plus(udev, table_SE_AIN);
  for (i = 0; i < NGAINS_USB1208FS_PLUS; i++ ) {
    for (j = 0; j < NCHAN_DE; j++) {
      printf("Calibration Table: Range = %d Channel = %d Slope = %f   Offset = %f\n", 
	     i, j, table_DE_AIN[i][j][0], table_DE_AIN[i][j][1]);
    }
  }
  for (i = 0; i < NCHAN_SE; i++ ) {
    printf("Calibration Single Ended Table: Channel = %d Slope = %f   Offset = %f\n", 
	   i, table_SE_AIN[i][0], table_SE_AIN[i][1]);
  }
  
  while(1) {
    printf("\nUSB 1208FS-Plus/USB 1408FS-Plus Testing\n");
    printf("----------------\n");
    printf("Hit 'b' to blink\n");
    printf("Hit 'c' to test counter\n");
    printf("Hit 'd' to test digitial IO\n");
    printf("Hit 'i' to test Analog Input\n");
    printf("Hit 'I' to test Analog Input Scan\n");
    printf("Hit 'o' to test Analog Output\n");
    printf("Hit 'O' to test Analog Scan Output\n");
    printf("Hit 'x' to test Analog Input Scan (Multi-channel)\n");
    printf("Hit 'r' to reset the device\n");
    printf("Hit 's' to get serial number\n");
    printf("Hit 'S' to get Status\n");
    printf("Hit 'e' to exit\n");

    while((ch = getchar()) == '\0' || ch == '\n');
    switch(ch) {
      case 'b': /* test to see if LED blinks */
        printf("Enter number or times to blink: ");
        scanf("%hhd", &options);
        usbBlink_USB1208FS_Plus(udev, options);
	break;
      case 'c':
        usbCounterInit_USB1208FS_Plus(udev);
        printf("Connect DIO Port A0 to CTR0\n");
	usbDTristateW_USB1208FS_Plus(udev, PORTA, 0x0);
	usbDLatchW_USB1208FS_Plus(udev, PORTA, 0x0);  // put pin 0 into known state
        toContinue();
        for (i = 0; i < 100; i++) {                   // toggle
	  usbDLatchW_USB1208FS_Plus(udev, PORTA, 0x1);
	  usbDLatchW_USB1208FS_Plus(udev, PORTA, 0x0);
        }
        printf("Count = %d.  Should read 100.\n", usbCounter_USB1208FS_Plus(udev));
        break;      
      case 'd':
        printf("\nTesting Digital I/O...\n");
	printf("connect PORTA <--> PORTB\n");
	usbDTristateW_USB1208FS_Plus(udev, PORTA, 0x0);
	printf("Digital Port Tristate Register = %#x\n", usbDTristateR_USB1208FS_Plus(udev, PORTA));
	do {
          printf("Enter a  number [0-0xff] : " );
          scanf("%x", &temp);
          usbDLatchW_USB1208FS_Plus(udev, PORTA, (__u8)temp);
	  temp = usbDLatchR_USB1208FS_Plus(udev, PORTA);
          input = usbDPort_USB1208FS_Plus(udev, PORTB);
          printf("The number you entered = %#x   Latched value = %#x\n\n",input, temp);
        } while (toContinue());
        break;
      case 'i':
	printf("Input channel [0-7]: ");
	scanf("%hhd", &channel);
        printf("Input range [0-7]: ");
	scanf("%hhd", &range);
	for (i = 0; i < 20; i++) {
	  value = usbAIn_USB1208FS_Plus(udev, channel, DIFFERENTIAL, range);
	  value = rint(value*table_DE_AIN[range][channel][0] + table_DE_AIN[range][channel][1]);
          if (device == USB1208FS_PLUS_PID) {
  	    printf("Range %d  Channel %d   Sample[%d] = %#x Volts = %lf\n",
		   range, channel,  i, value, volts_USB1208FS_Plus(udev, value, range));
	  } else {
  	    printf("Range %d  Channel %d   Sample[%d] = %#x Volts = %lf\n",
		   range, channel,  i, value, volts_USB1408FS_Plus(udev, value, range));

	  }
	  usleep(50000);	  
	}
        break;
      case 'I':
	printf("Testing USB-1208FS_Plus Analog Input Scan.\n");
	usbAInScanStop_USB1208FS_Plus(udev);
        printf("Enter number of scans (less than 512): ");
        scanf("%d", &count);
	printf("Input channel 0-7: ");
        scanf("%hhd", &channel);
        printf("Enter sampling frequency [Hz]: ");
	scanf("%lf", &frequency);
        printf("Enter Range [0-7]: ");
        scanf("%hhd", &range);
        ranges[channel] = range;
        options = DIFFERENTIAL_MODE;
        usbAInScanStop_USB1208FS_Plus(udev);
	usbAInScanClearFIFO_USB1208FS_Plus(udev);
        usbAInScanConfig_USB1208FS_Plus(udev, ranges);
	sleep(1);
        usbAInScanConfigR_USB1208FS_Plus(udev, ranges);
        for (i = 0; i < 4; i++) {
          printf("Channel %d     range %d\n", i, ranges[i]);
	}
	usbAInScanStart_USB1208FS_Plus(udev, count, 0x0, frequency, (0x1<<channel), options);
	ret = usbAInScanRead_USB1208FS_Plus(udev, count, 1, dataAIn);
	printf("Number samples read = %d\n", ret/2);
	for (i = 0; i < count; i++) {
	  dataAIn[i] = rint(dataAIn[i]*table_DE_AIN[range][channel][0] + table_DE_AIN[range][channel][1]);
          if (device == USB1208FS_PLUS_PID) {
	    printf("Range %d Channel %d  Sample[%d] = %#x Volts = %lf\n", range, channel,
		   i, dataAIn[i], volts_USB1208FS_Plus(udev,dataAIn[i],range));
	  } else {
	    printf("Range %d Channel %d  Sample[%d] = %#x Volts = %lf\n", range, channel,
		   i, dataAIn[i], volts_USB1408FS_Plus(udev,dataAIn[i],range));
	  }
	}
        break;
      case 'o':
        printf("Test Analog Output\n");
        printf("Enter Channel [0-1] ");
        scanf("%hhd", &channel);
        printf("Enter voltage: ");
	scanf("%lf", &voltage);
        value = voltage * 4096 / 5;
	usbAOut_USB1208FS_Plus(udev, channel, value);
	value = usbAOutR_USB1208FS_Plus(udev, channel);
	printf("Analog Output Voltage = %f V\n", volts_USB1208FS_Plus(udev, value, UP_5V));
        break;
      case 'O':
        printf("Test of Analog Output Scan. \n");
        printf("Hook scope up to VDAC 0\n");
        printf("Enter desired frequency of sine wave [Hz]: ");
        scanf("%lf", &frequency);
	frequency *= 2.;

        for (i = 0; i < 32; i++) {
          dataAOut[4*i] =   0x0;
	  dataAOut[4*i+1] = 0x800;
	  dataAOut[4*i+2] = 0xfff;
	  dataAOut[4*i+3] = 0xcff;
	}
	usbAOutScanStop_USB1208FS_Plus(udev);
        options = 0x3;   // output channel 0 and 1 output scan
	usbAOutScanStart_USB1208FS_Plus(udev, 0, frequency, options);
	printf("Hit \'s <CR>\' to stop ");
	flag = fcntl(fileno(stdin), F_GETFL);
	fcntl(0, F_SETFL, flag | O_NONBLOCK);
	do {
	  if (usb_bulk_write(udev, USB_ENDPOINT_OUT|2, (char *) dataAOut, sizeof(dataAOut), 1000) < 0) {
	    perror("usb_bulk_write error in AOutScan.");
	  }
	  //	  usb_bulk_write(udev, USB_ENDPOINT_OUT|2, (char *) dataAOut, 0x0, 400);
	} while (!isalpha(getchar()));
	fcntl(fileno(stdin), F_SETFL, flag);
	usbAOutScanStop_USB1208FS_Plus(udev);
	break;
      case 'x':
        printf("Testing USB-1208FS_Plus Mult-Channel Analog Input Scan.\n");
        usbAInScanStop_USB1208FS_Plus(udev);
        printf("enter number of channels (1-8) :");
        scanf("%d", &nchan);
        printf("Enter number of scans (less than 512): ");
        scanf("%d", &count);
        printf("Enter number of repeats: ");
        scanf("%d", &repeats);
        // Build bitmap for the first nchan in channels.
        channels = 0;
        for (i = 0; i < nchan; i++)
	  channels |= (1 << i);
        printf ("channels: %02X   count:%d\n", channels, count);
        frequency = 10000.;
        // Always use BP_20V to make it easy (BP_20V is 0...)
        range = 0;
        memset(ranges, 0, sizeof(ranges));
        usbAInScanConfig_USB1208FS_Plus(udev, ranges);
        // Run a loop for the specified number of repeats and
        // show the results...
        for (m = 0; m < repeats; m++) {
	  printf("\n\n---------------------------------------");
	  printf("\nrepeat: %d\n", m);
	  usbAInScanStart_USB1208FS_Plus(udev, count, 0x0, frequency, channels, 0);
	  ret = usbAInScanRead_USB1208FS_Plus(udev, count, nchan, dataAIn);
	  printf("Number samples read = %d\n", ret/2);
	  if (ret != count * nchan * 2) {
	    printf("***ERROR***  ret = %d   count = %d  nchan = %d\n", ret, count, nchan);
	    continue;
	  } /* if (ret != count * nchan * 2) */
	  for (i = 0; i < count; i++) {
	    printf("%6d", i);
	    for (j = 0; j < nchan; j++)	{
	      k = i*nchan + j;
	      data = rint(dataAIn[k]*table_DE_AIN[range][j][0] + table_DE_AIN[range][j][1]);
	      printf(", %8.4f", volts_USB1208FS_Plus(udev, data, range));
	    } /* for (j - 0; j < 8, j++) */
	    printf("\n");
	  } /* for (i = 0; i < count; i++) */
	} /* for (m = 0; m < repeats; m++) */
	printf("\n\n---------------------------------------");
	break;
      case 'r':
        usbReset_USB1208FS_Plus(udev);
        break;
      case 's':
        usbGetSerialNumber_USB1208FS_Plus(udev, serial);
        printf("Serial number = %s\n", serial);
        break;
      case 'S':
        printf("Status = %#x\n", usbStatus_USB1208FS_Plus(udev));
	break;
      case 'e':
        cleanup_USB1208FS_Plus(udev);
        return 0;
      default:
        break;
    }
  }
}

