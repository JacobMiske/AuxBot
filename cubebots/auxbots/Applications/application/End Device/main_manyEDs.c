/*----------------------------------------------------------------------------
 *  Demo Application for SimpliciTI
 *
 *  L. Friedman
 *  Texas Instruments, Inc.
 *---------------------------------------------------------------------------- */

/**********************************************************************************************
  Copyright 2007-2008 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights granted under
  the terms of a software license agreement between the user who downloaded the software,
  his/her employer (which must be your employer) and Texas Instruments Incorporated (the
  "License"). You may not use this Software unless you agree to abide by the terms of the
  License. The License limits your use, and you acknowledge, that the Software may not be
  modified, copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio frequency
  transceiver, which is integrated into your product. Other than for the foregoing purpose,
  you may not use, reproduce, copy, prepare derivative works of, modify, distribute,
  perform, display or sell this Software and/or its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS”
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
  WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
  IN NO EVENT SHALL TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE
  THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY
  INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST
  DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY
  THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
 **************************************************************************************************/

//includes for the radio
#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "nwk.h"

//user includes
#include "math.h"

/* work loop semaphores */
static volatile uint8_t RxBroadcastSem;

static void linkTo(void);

void toggleLED(uint8_t);

/* Callback handler */
static uint8_t sCB(linkID_t);

/* received message handler */
static void processMessage(linkID_t, uint8_t *, uint8_t);

static  linkID_t sLinkID1 = 0;

//define this device's unique address
//REMEMBER TO SET THE DEVICE ADDRESS TO WHAT YOU WANT IN THE "smpl_config_ED.dat" file!!! And to do a CLEAN and then Build, otherwise the address won't be properly compiled
// IMPORTANT: The last two bytes of this array will act as the identifier for this specific servo. It must be unique between servos.
addr_t lAddr = THIS_DEVICE_ADDRESS;

#define SPIN_ABOUT_A_SECOND   NWK_DELAY(1000)
#define SPIN_ABOUT_A_QUARTER_SECOND   NWK_DELAY(250)
#define CHANGE_DEFAULT_ROM_DEVICE_ADDRESS

#define NUM_MOTORS_PER_MESSAGE 6

/**********************************
 * user global variables
**********************************/
float amplitude = 0.0;
float frequency = 0.0;
float phase = 0.0;
float time = 0.0;
float start_time = 0.0;
#pragma DATA_ALIGN (RadioMSG, sizeof(int)); //align to int boundary so I can do nice pointer casts to get the data that I want
uint8_t     RadioMSG[MAX_APP_PAYLOAD];
/*********************************/

/**********************************
 * InitTimer
 * Setup Timer_A to output the PWM signal that we care about and to also keep our timebase for the
**********************************/
void InitTimer(void)
{
    P2DIR |= BIT3;                            // P2.3 is output
    P2SEL |= BIT3;                            // P2.3 is TACCR1 output
    TA0CCR0 = (PWMPeriod);  //
    TA0CCTL0 = CCIE;    // CCR0 interrupt enabled
    TA0CCR1 = PWMPeriod;        // CCR is not initialized
    TA0CCTL1 = (OUTMOD_3);  //set when we count to TACCR1, reset when we count to TACCR0
    TA0CTL = (TASSEL_2+ID_3+MC_1+TACLR);    //SMCLK,Div=8,UP to CCR0 mode,clear counter. Timer runs at 1MHz
}

/**********************************
 * Timer A0 CCR0 ISR
**********************************/
#pragma vector=TIMERA0_VECTOR
__interrupt void TIMERA0_ISR(void)
{
    time+=Timestep;
    //turn on the PWM generation is we're asking for a signal
    if (amplitude > 0) TA0CCR1=(PWMPeriod-1500)+(int)(500*amplitude*sinf(frequency*(time-start_time)+phase));
    else TA0CCR1=0;
}
/*********************************/


void main (void)
{
    uint8_t    done = 0;
    bspIState_t intState;
    unsigned char test_msg[9];

    RxBroadcastSem=0;

    BSP_Init(); //init the board
    InitTimer();    //setup the PWM generating timer

    //set the device address from the one declared in the *_config_XX.dat file. Must be done before call to SMPL_Init()
#ifdef CHANGE_DEFAULT_ROM_DEVICE_ADDRESS
    {
        //createRandomAddress(&lAddr);
        SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);
    }
#endif

    SMPL_Init(sCB);

    /* LEDs on solid to indicate successful join. */
    if (!BSP_LED2_IS_ON())
    {
        toggleLED(2);
    }
    if (!BSP_LED1_IS_ON())
    {
        toggleLED(1);
    }

    //we may not need this line (test and find out)
    SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RXON, 0);

    while (1)
    {

        // Have we received a broadcast msg?
        if (RxBroadcastSem)
        {
            uint8_t len;
            if (SMPL_SUCCESS == SMPL_Receive(SMPL_LINKID_USER_UUD, RadioMSG, &len))
            {
                processMessage(SMPL_LINKID_USER_UUD, RadioMSG, len);
                BSP_ENTER_CRITICAL_SECTION(intState);
                RxBroadcastSem--;
                BSP_EXIT_CRITICAL_SECTION(intState);
            }

        }
    }
}

void toggleLED(uint8_t which)
{
    if (1 == which)
    {
        BSP_TOGGLE_LED1();
    }
    else if (2 == which)
    {
        BSP_TOGGLE_LED2();
    }
    return;
}

static uint8_t sCB(linkID_t lid)
{
    //broadcast msg
    if (lid == SMPL_LINKID_USER_UUD)
    {
        RxBroadcastSem++;
        return 0;
    }

    return 1;
}

/***********************************************************
 * processMessage
 * Receive an aligned and formatted data message and decode it
 * Each motor gets 3 ints of data, amplitude, frequency and phase
 * that are packed in a byte array, little byte first as usual
***********************************************************/
static void processMessage(linkID_t lid, uint8_t *msg, uint8_t len)
{
    int tempAmp, tempFreq, tempPhase;
    float tempAmpF, tempFreqF, tempPhaseF;
    bspIState_t intState;
    unsigned int flag=0;
    //parse the received message and set the PWM numbers accordingly
    flag=*(unsigned int*)(msg+0);
    //be sure the inital message alignment is what we expect, that it matches an expected message type, and it's only 6 bytes
    if ((flag == TIME_MSG) && (lid == SMPL_LINKID_USER_UUD) && len == 6)
    {
        BSP_ENTER_CRITICAL_SECTION(intState);   //protect from possible interrupts until we've written all values that might be used by the ISR
        time = *(float*)(msg+2);    //jam the time parameter that we just received from the radio
        BSP_EXIT_CRITICAL_SECTION(intState);
        toggleLED(1);
    }
    else if (flag == MOT_MSG)
    {
        int offset = 0;

        uint8_t i;

        /************************************************************************************
                    Broadcast message diagram
                    +------+--+------+--+------+--+------+--+------+--+------+--+--+
                    |  6   |2 |  6   |2 |  6   |2 |  6   |2 |  6   |2 |  6   |2 |2 |   Tx
                    |      |  |      |  |      |  |      |  |      |  |      |  |  | ------>
                    +------+--+------+--+------+--+------+--+------+--+------+--+--+
        Bytes -->   49                                                 9    4  2 1 0

        Each broadcast has data for 6 motors.
        Each block of 6 bytes is instructions for a motor.
        Each block of 2 bytes is the ED address (last two bytes of lAddr).
        The right-most block of 2 bytes is the flag indicating this is a motor message (MOT_MSG).
        *************************************************************************************/

        // Scan the broadcast message to check if any broadcast addresses match the last two bytes of its own
        for(i = 0; i < NUM_MOTORS_PER_MESSAGE; i++)
        {
            if(lAddr.addr[2] == *(unsigned char*)(msg+8*i+2) && lAddr.addr[3] == *(unsigned char*)(msg+8*i+3))
                //compare the third and fourth bytes of ED address to the broadcast address bytes
            {
                offset = 8*i+4; // 8 bytes per set of motor instructions + address and 4 bytes at the beginning to offset
                break; // break out of loop if match found
            }
        }

/*
        if(lAddr.addr[2] == *(unsigned char*)(msg + 2) && lAddr.addr[3] == *(unsigned char*)(msg + 3))
        {
            offset = 4;
        }
        else if (lAddr.addr[2] == *(unsigned char*)(msg + 10) && lAddr.addr[3] == *(unsigned char*)(msg + 11))
        {
            offset = 12;
        }
        else if (lAddr.addr[2] == *(unsigned char*)(msg + 18) && lAddr.addr[3] == *(unsigned char*)(msg + 19))
        {
            offset = 20;
        }
        else if (lAddr.addr[2] == *(unsigned char*)(msg + 26) && lAddr.addr[3] == *(unsigned char*)(msg + 27))
        {
            offset = 28;
        }
        else if (lAddr.addr[2] == *(unsigned char*)(msg + 34) && lAddr.addr[3] == *(unsigned char*)(msg + 35))
        {
            offset = 36;
        }
        else if (lAddr.addr[2] == *(unsigned char*)(msg + 42) && lAddr.addr[3] == *(unsigned char*)(msg + 43))
        {
            offset = 44;
        } */

        if (offset != 0) // was a match found?
        {
            tempAmp=*(unsigned int*)(msg+offset);
            tempAmpF=((float)tempAmp)/500;  //convert to float and rescale since we prescaled to send as an int
            tempFreq=*(unsigned int*)(msg+offset+2);
            tempFreqF=((float)tempFreq)/500;
            tempPhase=*(unsigned int*)(msg+offset+4);
            tempPhaseF=((float)tempPhase)/500;

            //finally, copy to the system-level values and reset the time
            BSP_ENTER_CRITICAL_SECTION(intState);   //protect from possible interrupts until we've written all values that might be used by the ISR
            amplitude = tempAmpF;
            frequency = tempFreqF;
            phase = tempPhaseF;
            start_time = time;  //update the time at which we start the sinusoid so we have unambiguous phase
            BSP_EXIT_CRITICAL_SECTION(intState);
            toggleLED(2);
        }
    }

    return;
}

