/* SPDX-License-Identifier: BSD-3-Clause
 * AS3K payload-0, Motorola 68000 (not HC11).
 * SRAM 0x00030000; stack descends from 0x0003f000 (deepest call: 12 bytes).
 * Port the donor LCD_BYTE/LCD_NIBBLE/DELAY_LONG flow to AS3K Port C.
 * Initialize both existing KS0066, display BOOT OK on E1, then retain the
 * original identity, image checksum, binary echo and bootstrap reentry.
 */
        .cpu 68000
        .text
        .global _start,payload_end
_start:
        move.w #0x2700,%sr
        movea.l #0x0003f000,%sp
        bsr.w lcd_boot_ok
        lea identity(%pc),%a1
identify:
        move.b (%a1)+,%d0
        beq.s checksum
        bsr.s transmit
        bra.s identify
checksum:
        lea _start(%pc),%a1
        move.w #payload_end-_start-1,%d2
        moveq #0,%d4
        moveq #0,%d0
sum_loop:
        move.b (%a1)+,%d0
        add.l %d0,%d4
        dbra %d2,sum_loop
        moveq #3,%d2
sum_output:
        rol.l #8,%d4
        move.b %d4,%d0
        bsr.s transmit
        dbra %d2,sum_output
        moveq #3,%d2
challenge:
        btst #5,0xfffff904:w
        beq.s challenge
        move.b 0xfffff905:w,%d0
        bsr.s transmit
        dbra %d2,challenge
        lea complete(%pc),%a1
finish:
        move.b (%a1)+,%d0
        beq.s reenter
        bsr.s transmit
        bra.s finish
reenter:
        btst #7,0xfffff906:w
        beq.s reenter
        jmp 0xffffff44
transmit:
        btst #5,0xfffff906:w
        beq.s transmit
        move.b %d0,0xfffff907:w
        rts

/* PCSEL/PCDIR=FF: all eight pins GPIO outputs.
 * PC[3:0]=DB[7:4], PC4=RW (always zero), PC5=RS, PC6=E1, PC7=E2.
 * D5 enables; D6 RS; D1 retains the byte, D2 pulse value, D3 delay.
 * D7 is preserved for the existing IBUFF execution proof.
 */
lcd_boot_ok:
        move.b #0xff,0xfffff413:w
        move.b #0xff,0xfffff410:w
        clr.b 0xfffff411:w
        moveq #-64,%d5
        moveq #0,%d6
        bsr.w lcd_long
        moveq #3,%d0
        bsr.w lcd_nibble
        bsr.w lcd_long
        moveq #3,%d0
        bsr.w lcd_nibble
        bsr.w lcd_long
        moveq #3,%d0
        bsr.w lcd_nibble
        bsr.w lcd_long
        moveq #2,%d0
        bsr.w lcd_nibble
        moveq #0x28,%d0
        bsr.w lcd_byte
        moveq #0x08,%d0
        bsr.w lcd_byte
        moveq #0x01,%d0
        bsr.w lcd_byte
        bsr.w lcd_long
        moveq #0x06,%d0
        bsr.w lcd_byte
        moveq #0x0c,%d0
        bsr.w lcd_byte
        moveq #0x40,%d5
        move.w #0x80,%d0
        bsr.w lcd_byte
        moveq #0x20,%d6
        lea lcd_message(%pc),%a1
lcd_message_loop:
        move.b (%a1)+,%d0
        beq.s lcd_done
        bsr.w lcd_byte
        bra.s lcd_message_loop
lcd_done:
        clr.b 0xfffff411:w
        rts
lcd_byte:
        move.b %d0,%d1
        lsr.b #4,%d0
        bsr.w lcd_nibble
        move.b %d1,%d0
        bsr.w lcd_nibble
        rts
lcd_nibble:
        andi.b #0x0f,%d0
        or.b %d6,%d0
        move.b %d0,0xfffff411:w
        move.b %d0,%d2
        or.b %d5,%d2
        move.b %d2,0xfffff411:w
        nop
        nop
        move.b %d0,0xfffff411:w
        move.w #0x0100,%d3
lcd_short_loop:
        dbra %d3,lcd_short_loop
        rts
lcd_long:
        move.w #0x7fff,%d3
lcd_long_loop:
        dbra %d3,lcd_long_loop
        rts
lcd_message:
        .asciz "BOOT OK"
identity:
        .asciz "AS3K:P0:v1:BOOT OK\r\n"
complete:
        .asciz "OK\r\n"
        .balign 2
payload_end:
