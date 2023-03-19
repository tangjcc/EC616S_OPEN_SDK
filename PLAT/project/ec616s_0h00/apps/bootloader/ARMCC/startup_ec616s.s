;/**************************************************************************//**
; * @file     startup_ec616s.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM3 Device Series
; * @version  V5.00
; * @date     02. March 2016
; ******************************************************************************/
;/*
; * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
; *
; * SPDX-License-Identifier: Apache-2.0
; *
; * Licensed under the Apache License, Version 2.0 (the License); you may
; * not use this file except in compliance with the License.
; * You may obtain a copy of the License at
; *
; * www.apache.org/licenses/LICENSE-2.0
; *
; * Unless required by applicable law or agreed to in writing, software
; * distributed under the License is distributed on an AS IS BASIS, WITHOUT
; * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; * See the License for the specific language governing permissions and
; * limitations under the License.
; */

;/*
;//-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
;*/


; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00001000

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp
			
				EXPORT Stack_Size

                PRESERVE8
                THUMB

; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

                IF      :DEF:MIDDLEWARE_FOTA_ENABLE

Heap_Size       EQU     0x0026000

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
                EXPORT __heap_base
                EXPORT __heap_limit

__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                ENDIF

                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     Default_Handler          ; Reserved
                DCD     Default_Handler          ; Reserved
                DCD     Default_Handler          ; Reserved
                DCD     Default_Handler          ; Reserved
                DCD     Default_Handler		; SVCall Handler
                DCD     Default_Handler          ; Debug Monitor Handler
                DCD     Default_Handler          ; Reserved
                DCD     Default_Handler          ; PendSV Handler
                DCD     Default_Handler          ; SysTick Handler

                ; External Interrupts
                ;DCD     RTC_WakeupIntHandler	  ;  0:  RTC Wakeup
                ;DCD     Pad0_WakeupIntHandler     ;  1:  Pad0 Wakeup
                ;DCD     Pad1_WakeupIntHandler     ;  2:  Pad1 Wakeup
                ;DCD     Pad2_WakeupIntHandler     ;  3:  Pad2 Wakeup
                ;DCD     Pad3_WakeupIntHandler     ;  4:  Pad3 Wakeup
                ;DCD     Pad4_WakeupIntHandler     ;  5:  Pad4 Wakeup
                ;DCD     Pad5_WakeupIntHandler     ;  6:  Pad5 Wakeup
                
                ;DCD     XIC_IntHandler            ;  7:  Peripheral XIC IRQ
                ;DCD     XIC_IntHandler            ;  8:  Modem XIC IRQ

__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY


; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  SystemInit
                IMPORT  ec_main
                LDR     R0, =0x4400003C
                LDR     R1, =0x00000000
                STR     R1, [R0]

                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =ec_main
                BX      R0
                ENDP


; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP

HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                BL Default_Handler
                B       .
                ENDP
                
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                BL Default_Handler                
                B       .
                ENDP	
 
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
                BL Default_Handler                
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
                BL Default_Handler                
                B       .
                ENDP

				
Default_Handler PROC
		   BL HardFault_Hook_Handler
		   B       .
                ENDP

HardFault_Hook_Handler PROC
		EXPORT  HardFault_Hook_Handler   [WEAK]
		B .
		ENDP


__clr_240K_mem  PROC
		MOVS	R0, #0
		STM	R6!,{R0}
		ADDS	R5, R5, #1
		CMP	R5, #0xEC00
		BCC	__clr_240K_mem
		LDR     R0, =0x08080000
		LDR	SP, [R0,#0]
		MOVS	LR, R7
		BX      LR

		ENDP


                ALIGN


; User Initial Stack & Heap

;                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
;                EXPORT  __heap_base
;                EXPORT  __heap_limit

;                ELSE

;                IMPORT  __use_two_region_memory
;                EXPORT  __user_initial_stackheap

;__user_initial_stackheap PROC
;                LDR     R0, =  Heap_Mem
;                LDR     R1, =(Stack_Mem + Stack_Size)
;                LDR     R2, = (Heap_Mem +  Heap_Size)
;                LDR     R3, = Stack_Mem
;                BX      LR
;                ENDP

;                ALIGN

;                ENDIF


                END
