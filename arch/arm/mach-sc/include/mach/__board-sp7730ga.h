/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __GPIO_SCX35GA_H__
#define __GPIO_SCX35GA_H__

#ifndef __ASM_ARCH_BOARD_H
#error  "Don't include this file directly, include <mach/board.h>"
#endif

#define GPIO_TOUCH_RESET         81
#define GPIO_TOUCH_IRQ           82

#define GPIO_SENSOR_RESET        186//41
#define GPIO_MAIN_SENSOR_PWN     187//42
#define GPIO_SUB_SENSOR_PWN      188//43

#define SPRD_FLASH_OFST          0x890
#define SPRD_FLASH_CTRL_BIT      0x8000
#define SPRD_FLASH_LOW_VAL       0x3
#define SPRD_FLASH_HIGH_VAL      0xF
#define SPRD_FLASH_LOW_CUR       110
#define SPRD_FLASH_HIGH_CUR      470

#define USB_OTG_CABLE_DETECT     72

#define EIC_CHARGER_DETECT		(A_EIC_START + 0)
#define EIC_POWER_PBINT2        (A_EIC_START + 1)
#define EIC_POWER_PBINT         (A_EIC_START + 2)
#define EIC_AUD_HEAD_BUTTON     (A_EIC_START + 3)
#define EIC_CHG_CV_STATE       (A_EIC_START + 4)
#define EIC_AUD_HEAD_INST2      (A_EIC_START + 5)
#define EIC_VCHG_OVI            (A_EIC_START + 6)
#define EIC_VBAT_OVI            (A_EIC_START + 7)

#define HEADSET_DETECT_GPIO_ACTIVE_LOW 0

#define EIC_KEY_POWER           (EIC_POWER_PBINT)
#define HEADSET_BUTTON_GPIO		(EIC_AUD_HEAD_BUTTON)
#define HEADSET_DETECT_GPIO		(EIC_AUD_HEAD_INST2)
#define HEADSET_SWITCH_GPIO	237

#define SPI0_CMMB_CS_GPIO        156
#define SPI1_WIFI_CS_GPIO        44

#define GPIO_BK                  136

#define GPIO_CMMB_RESET         144
#define GPIO_CMMB_INT           143
#define GPIO_CMMB_26M_CLK_EN    197

#define GPIO_BT_RESET       233
#define GPIO_BT_POWER       231
#define GPIO_BT2AP_WAKE     232
#define GPIO_AP2BT_WAKE     235

#define GPIO_WIFI_SHUTDOWN	230
#define GPIO_WIFI_IRQ		234

#define GPIO_PROX_INT 216
#define GPIO_GYRO_INT1 163
#define GPIO_M_RSTN	161
#define GPIO_M_DRDY 164

#define GPIO_SDIO_DETECT      75

/* ION config info
LCD:
   960x540

Video Playing:
   1080p 8 ref-frame

Video Recording:
   720p

Camera:
   8M,  support ZSL, support preview/capture rotation
*/
#define SPRD_ION_MM_SIZE         (52 * SZ_1M)
#define SPRD_ION_OVERLAY_SIZE    (4 * SZ_1M)

#endif
