/****************************************************************************
 *
 * MODULE:             SMBus functions
 *
 * COMPONENT:          $RCSfile: SMBus.c,v $
 *
 * VERSION:            $Name: RD_RELEASE_6thMay09 $
 *
 * REVISION:           $Revision: 1.2 $
 *
 * DATED:              $Date: 2008/02/29 18:02:05 $
 *
 * STATUS:             $State: Exp $
 *
 * AUTHOR:             Lee Mitchell
 *
 * DESCRIPTION:
 * A set of functions to communicate with devices on the SMBus
 *
 * CHANGE HISTORY:
 *
 * $Log: SMBus.c,v $
 * Revision 1.2  2008/02/29 18:02:05  dclar
 * dos2unix
 *
 * Revision 1.1  2006/12/08 10:51:17  lmitch
 * Added to repository
 *
 *
 *
 * LAST MODIFIED BY:   $Author: dclar $
 *                     $Modtime: $
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on
 * each copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2005, 2006. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE bool_t bSMBusWait(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void vSMBusInit(void)
{

	/* run bus at 100KHz */
	vAHI_SiMasterConfigure(TRUE, FALSE, 31);
			// 16/[(PreScaler + 1) x 5]MHz
			//		--> 31:100KHz, 7:400KHz

}


PUBLIC bool_t bSMBusWrite(uint8 u8Address, uint8 u8Command, uint8 u8Length, uint8* pu8Data)
{

	bool_t bCommandSent = FALSE;

	/* Send address with write bit set */
	vAHI_SiMasterWriteSlaveAddr(u8Address, E_AHI_SI_SLAVE_RW_SET);

	vAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bSMBusWait()) return(FALSE);

	while(bCommandSent == FALSE || u8Length > 0){

		if(!bCommandSent){

			/* Send command byte */
			vAHI_SiMasterWriteData8(u8Command);
			bCommandSent = TRUE;

		} else {

			u8Length--;

			/* Send data byte */
			vAHI_SiMasterWriteData8(*pu8Data++);

		}

		/*
		 * If its the last byte to be sent, send with
		 * stop sequence set
		 */
		if(u8Length == 0){

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_STOP_BIT,
							 E_AHI_SI_NO_SLAVE_READ,
							 E_AHI_SI_SLAVE_WRITE,
							 E_AHI_SI_SEND_ACK,
							 E_AHI_SI_NO_IRQ_ACK);

		} else {

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_NO_STOP_BIT,
							 E_AHI_SI_NO_SLAVE_READ,
							 E_AHI_SI_SLAVE_WRITE,
							 E_AHI_SI_SEND_ACK,
							 E_AHI_SI_NO_IRQ_ACK);

		}

		if(!bSMBusWait()) return(FALSE);

	}

	return(TRUE);

}


PUBLIC bool_t bSMBusRandomRead(uint8 u8Address, uint8 u8Command, uint8 u8Length, uint8* pu8Data)
{

	/* Send address with write bit set */
	vAHI_SiMasterWriteSlaveAddr(u8Address, !E_AHI_SI_SLAVE_RW_SET);

	vAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bSMBusWait()) return(FALSE);


	/* Send command byte */
	vAHI_SiMasterWriteData8(u8Command);
	vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bSMBusWait()) return(FALSE);



	while(u8Length > 0){

		u8Length--;

		/*
		 * If its the last byte to be sent, send with
		 * stop sequence set
		 */
		if(u8Length == 0){

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_STOP_BIT,
							 E_AHI_SI_SLAVE_READ,
							 E_AHI_SI_NO_SLAVE_WRITE,
							 E_AHI_SI_SEND_NACK,
							 E_AHI_SI_NO_IRQ_ACK);

		} else {

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_NO_STOP_BIT,
							 E_AHI_SI_NO_SLAVE_READ,
							 E_AHI_SI_SLAVE_WRITE,
							 E_AHI_SI_SEND_ACK,
							 E_AHI_SI_NO_IRQ_ACK);

		}

		while(bAHI_SiMasterPollTransferInProgress()); /* busy wait */

		*pu8Data++ = u8AHI_SiMasterReadData8();

	}

	return(TRUE);

}


PUBLIC bool_t bSMBusSequentialRead(uint8 u8Address, uint8 u8Length, uint8* pu8Data)
{

	/* Send address with write bit set */
	vAHI_SiMasterWriteSlaveAddr(u8Address, !E_AHI_SI_SLAVE_RW_SET);

	vAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bSMBusWait()) return(FALSE);


	while(u8Length > 0){

		u8Length--;

		/*
		 * If its the last byte to be sent, send with
		 * stop sequence set
		 */
		if(u8Length == 0){

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_STOP_BIT,
							 E_AHI_SI_SLAVE_READ,
							 E_AHI_SI_NO_SLAVE_WRITE,
							 E_AHI_SI_SEND_NACK,
							 E_AHI_SI_NO_IRQ_ACK);

		} else {

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_NO_STOP_BIT,
							 E_AHI_SI_SLAVE_READ,
							 E_AHI_SI_NO_SLAVE_WRITE,
							 E_AHI_SI_SEND_ACK,
							 E_AHI_SI_NO_IRQ_ACK);

		}

		while(bAHI_SiMasterPollTransferInProgress()); /* busy wait */

		*pu8Data++ = u8AHI_SiMasterReadData8();

	}

	return(TRUE);

}

bool_t bSMBusSequentialRead_NACK(uint8 u8Address, uint8 u8Length, uint8* pu8Data)
{

	/* Send address with write bit set */
	vAHI_SiMasterWriteSlaveAddr(u8Address, !E_AHI_SI_SLAVE_RW_SET);

	vAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bSMBusWait()) return(FALSE);


	while(u8Length > 0){

		u8Length--;

		/*
		 * If its the last byte to be sent, send with
		 * stop sequence set
		 */
		if(u8Length == 0){

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_STOP_BIT,
							 E_AHI_SI_SLAVE_READ,
							 E_AHI_SI_NO_SLAVE_WRITE,
							 E_AHI_SI_SEND_NACK,
							 E_AHI_SI_NO_IRQ_ACK);

		} else {

			vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_NO_STOP_BIT,
							 E_AHI_SI_SLAVE_READ,
							 E_AHI_SI_NO_SLAVE_WRITE,
							 E_AHI_SI_SEND_NACK,
							 E_AHI_SI_NO_IRQ_ACK);

		}

		while(bAHI_SiMasterPollTransferInProgress()); /* busy wait */

		*pu8Data++ = u8AHI_SiMasterReadData8();

	}

	return(TRUE);

}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

PRIVATE bool_t bSMBusWait(void)
{

	while(bAHI_SiMasterPollTransferInProgress()); /* busy wait */

	if (bAHI_SiMasterPollArbitrationLost() | bAHI_SiMasterCheckRxNack())	{

		/* release bus & abort */
		vAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						 E_AHI_SI_STOP_BIT,
						 E_AHI_SI_NO_SLAVE_READ,
						 E_AHI_SI_SLAVE_WRITE,
						 E_AHI_SI_SEND_ACK,
						 E_AHI_SI_NO_IRQ_ACK);
		return(FALSE);
	}

	return(TRUE);

}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
