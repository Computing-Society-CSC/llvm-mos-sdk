; Copyright (c) 2023 Rhys Weatherley
;
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions,
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

.include "imag.inc"

; Initialize the system from the reset vector.
.global __do_reset
.section .init.50,"ax",@progbits
__do_reset:
; Fix the D and I flags.
  cld
  cli
; Set up the initial 6502 stack pointer.
  ldx #$ff
  txs

  jsr _via_init

.text
; IRQBRK handler.
.global _irqbrk
.section .text._irqbrk,"axR",@progbits
_irqbrk:
  pha                   ; __systick_isr and __serial_isr use A and X.
  phx
  cld                   ; Just in case.
  jsr __systick_isr     ; Handle the system millisecond tick timer interrupt.
  plx
  pla
  jmp irq               ; Jump to the user-supplied IRQ handler.

; Default IRQ and NMI handler if the user's program hasn't defined one.
.global _irq_default
.section .text._irq_default,"axR",@progbits
_irq_default:
  rti


VIA_BASE = $6000
VIA_DDRB_REG =  VIA_BASE + $02 ; Data Direction Register B
VIA_DDRA_REG =  VIA_BASE + $03 ; Data Direction Register A
VIA_ACR_REG  =  VIA_BASE + $0B ; Auxiliary Control Register
VIA_PCR_REG  =  VIA_BASE + $0C ; Peripheral Control Register
VIA_IFR_REG  =  VIA_BASE + $0D ; Interrupt Flag Register
VIA_IER_REG  =  VIA_BASE + $0E ; Interrupt Enable Register

; ACR bits
VIA_ACR_T2_MODE_BIT   =   %00100000 ; Bit 5: Timer 2 Control - RESERVED FOR SYSTICK
VIA_TIMER1_ONE_SHOT_INTERRUPT = %10000000 ; T1 Mode: One-shot interrupt (ACR bits 7=1, 6=0)
VIA_IFR_CLEAR_MASK  = $7F                    ; = $7F (Mask for clearing user flags via IFR write)
VIA_IER_DISABLE_MASK = $5F                    ; = $5F (Mask for disabling user interrupts via IER write with bit 7=0)


; =============================================================================
; via_init() - Initialize VIA chip at reset (Assembly Version)
; Preserves Timer 2 settings (ACR bit 5, IER bit 5).
; Clobbers: A
; =============================================================================
.section .text._via_init,"ax",@progbits
_via_init:
    ; Set Ports A & B to input (Write 0 to DDRs)
    lda #0
    sta VIA_DDRA_REG
    sta VIA_DDRB_REG

    ; Configure ACR: Disable latches, disable SR, set T1 to one-shot interrupt
    ; PRESERVE T2 mode bit (Bit 5)
    lda #VIA_ACR_T2_MODE_BIT    ; Keep only the T2 mode bit (Bit 5)
    sta VIA_ACR_REG             ; Write the modified value back

    ; Configure PCR: Set CA1/CB1 to neg edge, CA2/CB2 to neg edge interrupt input mode
    ; (Interrupts themselves are disabled by IER setting below)
    ; Using value derived from datasheet Table 2-5: %00100010 = $22
    lda #%00100010              ; CA2=NegInt, CB2=NegInt, CA1=NegEdge, CB1=NegEdge
    sta VIA_PCR_REG

    ; Disable all user interrupts in IER (Preserves T2 enable bit)
    ; Write the mask VIA_IER_DISABLE_MASK ($5F) to IER.
    ; Since bit 7 is 0, this clears the enable bits corresponding to the 1s in the mask.
    lda #VIA_IER_DISABLE_MASK   ; Load mask ($5F)
    sta VIA_IER_REG             ; Write to IER (disables T1, SR, CA1, CA2, CB1, CB2)

    ; Clear all pending user interrupt flags in IFR (Preserves T2 flag bit)
    ; Write 1s to the flag bits to clear them.
    lda #VIA_IFR_CLEAR_MASK     ; Load mask ($7F)
    sta VIA_IFR_REG             ; Write to IFR (clears T1, SR, CA1, CA2, CB1, CB2 flags)

    rts                         ; Return from subroutine
