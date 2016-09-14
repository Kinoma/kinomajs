/**************************************************************************//**
 * @file     mw300.h
 * @brief    CMSIS Cortex-M4 Core Peripheral Access Layer Header File for
 *           Device MW300
 * @version  V5.00
 * @date     02. March 2016
 ******************************************************************************/
/*
 * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MW300_H
#define MW300_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup Marvell
  * @{
  */

/** @addtogroup MW300
  * @{
  */

/** @addtogroup Configuration_of_CMSIS
  * @{
  */

/* =========================================================================================================================== */
/* ================                                Interrupt Number Definition                                ================ */
/* =========================================================================================================================== */

typedef enum IRQn
{
/* =======================================  ARM Cortex-M4 Specific Interrupt Numbers  ======================================== */
  Reset_IRQn                = -15,              /*!< -15  Reset Vector, invoked on Power up and warm reset                     */
  NonMaskableInt_IRQn       = -14,              /*!< -14  Non maskable Interrupt, cannot be stopped or preempted               */
  HardFault_IRQn            = -13,              /*!< -13  Hard Fault, all classes of Fault                                     */
  MemoryManagement_IRQn     = -12,              /*!< -12  Memory Management, MPU mismatch, including Access Violation
                                                          and No Match                                                         */
  BusFault_IRQn             = -11,              /*!< -11  Bus Fault, Pre-Fetch-, Memory Access Fault, other address/memory
                                                          related Fault                                                        */
  UsageFault_IRQn           = -10,              /*!< -10  Usage Fault, i.e. Undef Instruction, Illegal State Transition        */
  SVCall_IRQn               =  -5,              /*!< -5 System Service Call via SVC instruction                                */
  DebugMonitor_IRQn         =  -4,              /*!< -4 Debug Monitor                                                          */
  PendSV_IRQn               =  -2,              /*!< -2 Pendable request for system service                                    */
  SysTick_IRQn              =  -1,              /*!< -1 System Tick Timer                                                      */

/* ===========================================  MW300 Specific Interrupt Numbers  ========================================= */
  USBC_IRQn                 = 29,                /*!< Device Interrupt                                                         */
} IRQn_Type;


/* =========================================================================================================================== */
/* ================                           Processor and Core Peripheral Section                           ================ */
/* =========================================================================================================================== */

/* ===========================  Configuration of the ARM Cortex-M4 Processor and Core Peripherals  =========================== */
/* ToDo: set the defines according your Device */
/* ToDo: define the correct core revision
         __CM0_REV if your device is a Cortex-M0 device
         __CM3_REV if your device is a Cortex-M3 device
         __CM4_REV if your device is a Cortex-M4 device
         __CM7_REV if your device is a Cortex-M7 device */
#define __CM4_REV                 0x0201    /*!< Core Revision r2p1 */
/* ToDo: define the correct core features for the <Device> */
#define __MPU_PRESENT             1         /*!< Set to 1 if MPU is present */
#define __VTOR_PRESENT            1         /*!< Set to 1 if VTOR is present */
#define __NVIC_PRIO_BITS          3         /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0         /*!< Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT             0         /*!< Set to 1 if FPU is present */
#define __FPU_DP                  0         /*!< Set to 1 if FPU is double precision FPU (default is single precision FPU) */
#define __ICACHE_PRESENT          0         /*!< Set to 1 if I-Cache is present */
#define __DCACHE_PRESENT          0         /*!< Set to 1 if D-Cache is present */
#define __DTCM_PRESENT            0         /*!< Set to 1 if DTCM is present */


/** @} */ /* End of group Configuration_of_CMSIS */


#include <core_cm4.h>                           /*!< ARM Cortex-M4 processor and core peripherals */
//#include "system_mw300.h"                    /*!< <Device> System */


/* ========================================  Start of section using anonymous unions  ======================================== */
#if   defined (__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif


/* =========================================================================================================================== */
/* ================                            Device Specific Peripheral Section                             ================ */
/* =========================================================================================================================== */

#define _BITFIELD(x, s)             ((uint32_t) (x << s))
#define _BITFIELD_1(s)              _BITFIELD(1, s)
#define _BITFIELD_2(x, s)           _BITFIELD(x & 0x03, s)
#define _BITFIELD_3(x, s)           _BITFIELD(x & 0x07, s)
#define _BITFIELD_4(x, s)           _BITFIELD(x & 0x0f, s)
#define _BITFIELD_8(x, s)           _BITFIELD(x & 0xff, s)
#define _BITFIELD_16(x, s)          _BITFIELD(x & 0xffff, s)

/** @addtogroup Device_Peripheral_peripherals
  * @{
  */

/**
  * @brief USB (USBC)
  */

typedef struct
{
  __IM  uint32_t   ID;
  __IM  uint32_t   HWGENERAL;
  __IM  uint32_t   HWHOST;
  __IM  uint32_t   HWDEVICE;
  __IM  uint32_t   HWTXBUF;
  __IM  uint32_t   HWRXBUF;
        uint32_t   RESERVED0[26];
  __IOM uint32_t   GPTIMER0LD;
  __IOM uint32_t   GPTIMER0CTRL;
  __IOM uint32_t   GPTIMER1LD;
  __IOM uint32_t   GPTIMER1CTRL;
  __IOM uint32_t   SBUSCFG;
        uint32_t   RESERVED1[27];
  __IM  uint32_t   CAPLENGTH_HCIVERSION;
  __IM  uint32_t   HCSPARAMS;
  __IM  uint32_t   HCCPARAMS;
        uint32_t   RESERVED2[5];
  __IM  uint32_t   DCIVERSION;
  __IM  uint32_t   DCCPARAMS;
        uint32_t   RESERVED3[6];
  __IOM uint32_t   USBCMD;
  __IOM uint32_t   USBSTS;
  __IOM uint32_t   USBINTR;
  __IOM uint32_t   FRINDEX;
        uint32_t   RESERVED4;
  union {
    __IOM uint32_t PERIODICLISTBASE;
    __IOM uint32_t DEVICEADDR;
  };
  union {
    __IOM uint32_t ASYNCLISTADDR;
    __IOM uint32_t ENDPTLISTADDR;
  };
  __IOM uint32_t   TTCTRL;
  __IOM uint32_t   BURSTSIZE;
  __IOM uint32_t   TXFILLTUNING;
  __IOM uint32_t   TXTTFILLTUNING;
  __IOM uint32_t   IC_USB;
  __IOM uint32_t   ULPI_VIEWPORT;
        uint32_t   RESERVED5;
  __IOM uint32_t   ENDPTNAK;
  __IOM uint32_t   ENDPTNAKEN;
        uint32_t   RESERVED6;
  __IOM uint32_t   PORTSC1;
        uint32_t   RESERVED7[7];
  __IOM uint32_t   OTGSC;
  __IOM uint32_t   USBMODE;
  __IOM uint32_t   ENDPTSETUPSTAT;
  __IOM uint32_t   ENDPTPRIME;
  __IOM uint32_t   ENDPTFLUSH;
  __IM  uint32_t   ENDPTSTAT;
  __IOM uint32_t   ENDPTCOMPLETE;
  __IOM uint32_t   ENDPTCTRL[8];
} MW300_USBC_TypeDef;

#define USBC_USBCMD_RS              _BITFIELD_1(0)
#define USBC_USBCMD_RST             _BITFIELD_1(1)
#define USBC_USBCMD_FS_1(x)         _BITFIELD_2(x, 2)
#define USBC_USBCMD_PSE             _BITFIELD_1(4)
#define USBC_USBCMD_ASE             _BITFIELD_1(5)
#define USBC_USBCMD_IAA             _BITFIELD_1(6)
#define USBC_USBCMD_LR              _BITFIELD_1(7)
#define USBC_USBCMD_ASP(x)          _BITFIELD_2(x, 8)
#define USBC_USBCMD_ASPE            _BITFIELD_1(11)
#define USBC_USBCMD_SUTW            _BITFIELD_1(13)
#define USBC_USBCMD_ATDTW           _BITFIELD_1(14)
#define USBC_USBCMD_FS_2            _BITFIELD_1(15)
#define USBC_USBCMD_ITC             _BITFIELD_8(16)

#define USBC_USBSTS_UI              _BITFIELD_1(0)
#define USBC_USBSTS_UEI             _BITFIELD_1(1)
#define USBC_USBSTS_PCI             _BITFIELD_1(2)
#define USBC_USBSTS_FRI             _BITFIELD_1(3)
#define USBC_USBSTS_SEI             _BITFIELD_1(4)
#define USBC_USBSTS_AAI             _BITFIELD_1(5) 
#define USBC_USBSTS_URI             _BITFIELD_1(6)
#define USBC_USBSTS_SRI             _BITFIELD_1(7)
#define USBC_USBSTS_SLI             _BITFIELD_1(8)
#define USBC_USBSTS_ULPII           _BITFIELD_1(10)
#define USBC_USBSTS_UALTI           _BITFIELD_1(11)
#define USBC_USBSTS_HCH             _BITFIELD_1(12)
#define USBC_USBSTS_RCL             _BITFIELD_1(13)
#define USBC_USBSTS_PS              _BITFIELD_1(14)
#define USBC_USBSTS_AS              _BITFIELD_1(15)
#define USBC_USBSTS_NAKI            _BITFIELD_1(16)
#define USBC_USBSTS_UAI             _BITFIELD_1(18)
#define USBC_USBSTS_UPI             _BITFIELD_1(19)
#define USBC_USBSTS_TI0             _BITFIELD_1(24)
#define USBC_USBSTS_TI1             _BITFIELD_1(25)

#define USBC_USBINTR_UE             _BITFIELD_1(0)
#define USBC_USBINTR_UEE            _BITFIELD_1(1)
#define USBC_USBINTR_PCE            _BITFIELD_1(2)
#define USBC_USBINTR_FRE            _BITFIELD_1(3)
#define USBC_USBINTR_SEE            _BITFIELD_1(4)
#define USBC_USBINTR_AAE            _BITFIELD_1(5) 
#define USBC_USBINTR_URE            _BITFIELD_1(6)
#define USBC_USBINTR_SRE            _BITFIELD_1(7)
#define USBC_USBINTR_SLE            _BITFIELD_1(8)
#define USBC_USBINTR_ULPIE          _BITFIELD_1(10)
#define USBC_USBINTR_UALTIE         _BITFIELD_1(11)
#define USBC_USBINTR_NAKE           _BITFIELD_1(16)
#define USBC_USBINTR_UAIE           _BITFIELD_1(18)
#define USBC_USBINTR_UPIE           _BITFIELD_1(19)
#define USBC_USBINTR_TIE0           _BITFIELD_1(24)
#define USBC_USBINTR_TIE1           _BITFIELD_1(25)

#define USBC_PORTSC1_CCS            _BITFIELD_1(0)
#define USBC_PORTSC1_CSC            _BITFIELD_1(1)
#define USBC_PORTSC1_PE             _BITFIELD_1(2)
#define USBC_PORTSC1_PEC            _BITFIELD_1(3)
#define USBC_PORTSC1_OCA            _BITFIELD_1(4)
#define USBC_PORTSC1_OCC            _BITFIELD_1(5)
#define USBC_PORTSC1_FPR            _BITFIELD_1(6)
#define USBC_PORTSC1_SUSP           _BITFIELD_1(7)
#define USBC_PORTSC1_PR             _BITFIELD_1(8)
#define USBC_PORTSC1_HSP            _BITFIELD_1(9)
#define USBC_PORTSC1_LS(x)          _BITFIELD_2(x, 10)
#define USBC_PORTSC1_PP             _BITFIELD_1(12)
#define USBC_PORTSC1_PO             _BITFIELD_1(13)
#define USBC_PORTSC1_PIC(x)         _BITFIELD_2(14)
#define USBC_PORTSC1_PTC(x)         _BITFIELD_4(16)
#define USBC_PORTSC1_WKCN           _BITFIELD_1(20)
#define USBC_PORTSC1_WKDS           _BITFIELD_1(21)
#define USBC_PORTSC1_WKOC           _BITFIELD_1(22)
#define USBC_PORTSC1_PHCD           _BITFIELD_1(23)
#define USBC_PORTSC1_PFSC           _BITFIELD_1(24)
#define USBC_PORTSC1_PTS2           _BITFIELD_1(25)
#define USBC_PORTSC1_PSPD(x)        _BITFIELD_2(x, 26)
#define USBC_PORTSC1_PTW            _BITFIELD_1(28)
#define USBC_PORTSC1_STS            _BITFIELD_1(29)
#define USBC_PORTSC1_PTS(x)         _BITFIELD_2(x, 30)

#define USBC_USBMODE_CM(x)          _BITFIELD_2(x, 0)
#define USBC_USBMODE_ES             _BITFIELD_1(2)
#define USBC_USBMODE_SLOM           _BITFIELD_1(3)
#define USBC_USBMODE_SDIS           _BITFIELD_1(4)
#define USBC_USBMODE_VBPS           _BITFIELD_1(5)
#define USBC_USBMODE_SRT            _BITFIELD_1(15)

#define USBC_ENDPTCTRL_RXS          _BITFIELD_1(0)
#define USBC_ENDPTCTRL_RXD          _BITFIELD_1(1)
#define USBC_ENDPTCTRL_RXT(x)       _BITFIELD_2(x, 2)
#define USBC_ENDPTCTRL_RXI          _BITFIELD_1(5)
#define USBC_ENDPTCTRL_RXR          _BITFIELD_1(6)
#define USBC_ENDPTCTRL_RXE          _BITFIELD_1(7)
#define USBC_ENDPTCTRL_TXS          _BITFIELD_1(0 + 16)
#define USBC_ENDPTCTRL_TXD          _BITFIELD_1(1 + 16)
#define USBC_ENDPTCTRL_TXT(x)       _BITFIELD_2(x, 2 + 16)
#define USBC_ENDPTCTRL_TXI          _BITFIELD_1(5 + 16)
#define USBC_ENDPTCTRL_TXR          _BITFIELD_1(6 + 16)
#define USBC_ENDPTCTRL_TXE          _BITFIELD_1(7 + 16)

/*@}*/ /* end of group Device_Peripheral_peripherals */

/* =========================================  End of section using anonymous unions  ========================================= */
#if   defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif


/* =========================================================================================================================== */
/* ================                          Device Specific Peripheral Address Map                           ================ */
/* =========================================================================================================================== */

/** @addtogroup Device_Peripheral_peripheralAddr
  * @{
  */

#define MW300_USBC_BASE   (0x44001000UL)    /*!< USB controller base address */

/** @} */ /* End of group Device_Peripheral_peripheralAddr */

/* =========================================================================================================================== */
/* ================                                  Peripheral declaration                                   ================ */
/* =========================================================================================================================== */

/** @addtogroup Device_Peripheral_declaration
  * @{
  */

#define MW300_USBC        ((MW300_USBC_TypeDef *) MW300_USBC_BASE)

/** @} */ /* End of group Device_Peripheral_declaration */


/** @} */ /* End of group MW300 */

/** @} */ /* End of group Marvell */

#ifdef __cplusplus
}
#endif

#endif  /* MW300_H */
