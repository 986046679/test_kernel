/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

/* #include "Mp_Precomp.h" */
/* #include "../odm_precomp.h" */

#include <drv_types.h>
#include "HalEfuseMask8821C_PCIE.h"



/******************************************************************************
*						   MPCIE.TXT
******************************************************************************/

u8 Array_MP_8821C_MPCIE[] = {
	0xFF,
	0xF3,
	0xEF,
	0x9E,
	0x70,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x03,
	0xF7,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xF1,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,

};

u16
EFUSE_GetArrayLen_MP_8821C_MPCIE(void)
{
	return sizeof(Array_MP_8821C_MPCIE) / sizeof(u8);
}

void
EFUSE_GetMaskArray_MP_8821C_MPCIE(
		u8 *Array
)
{
	u16 len = EFUSE_GetArrayLen_MP_8821C_MPCIE(), i = 0;

	for (i = 0; i < len; ++i)
		Array[i] = Array_MP_8821C_MPCIE[i];
}
BOOLEAN
EFUSE_IsAddressMasked_MP_8821C_MPCIE(
		u16 Offset
)
{
	int r = Offset / 16;
	int c = (Offset % 16) / 2;
	int result = 0;

	if (c < 4) /* Upper double word */
		result = (Array_MP_8821C_MPCIE[r] & (0x10 << c));
	else
		result = (Array_MP_8821C_MPCIE[r] & (0x01 << (c - 4)));

	return (result > 0) ? 0 : 1;
}
