/* SPDX-License-Identifier: BSD-3-Clause
 * Independently authored executable service model of MC68EZ328UM ch.16.
 * NOT a dump or reconstruction of Motorola's mask ROM. All I/O uses the
 * existing UART registers and bit timing. No host parser / PC assignment.
 * The generated image is mapped only when EMUBRK was low during reset.
 * AS3K's configured 16 MHz clock gives 9615 baud with UBAUD=0x010d.
 * The initial link character is consumed and acknowledged with '@'.
 */
        .cpu 68000
        .text
        .global model_start, model_end
        .equ URXH, 0xfffff904
        .equ URXL, 0xfffff905
        .equ UTXH, 0xfffff906
        .equ UTXL, 0xfffff907
model_start:
        move.w #0x010d,0xfffff902:w
        move.w #0xe100,0xfffff900:w
        andi.b #0xcf,0xfffff423:w
link_wait:
        btst #5,URXH:w
        beq.s link_wait
        move.b URXL:w,%d0
        move.b #0x40,UTXL:w
        bra.s new_record
        .org 0x44
new_record:
        moveq #8,%d2
        moveq #0,%d1
        moveq #0,%d5
receive:
        btst #5,URXH:w
        beq.s receive
        moveq #0,%d0
        move.b URXL:w,%d0
        move.b %d0,%d6
        cmpi.b #0x30,%d0
        bcs.s restart_echo
        cmpi.b #0x39,%d0
        bls.s decimal
        cmpi.b #0x41,%d0
        bcs.s restart_echo
        cmpi.b #0x46,%d0
        bhi.s restart_echo
        subq.b #7,%d0
decimal:
        subi.b #0x30,%d0
        lsl.l #4,%d1
        or.b %d0,%d1
        subq.b #1,%d2
        bne.s echo
        addq.b #1,%d5
        cmpi.b #1,%d5
        beq.s address_ready
        cmpi.b #2,%d5
        beq.s count_ready
        moveq #2,%d5
        move.b %d1,(%a0)+
        subq.b #1,%d3
        beq.s restart_echo
next_byte:
        moveq #2,%d2
        moveq #0,%d1
        bra.s echo
        /* Eight writable instruction bytes, followed by loader reentry. */
        .org 0xaa
        .space 8,0
buffer_return:
        bra.s new_record
address_ready:
        movea.l %d1,%a0
        bra.s next_byte
count_ready:
        move.b %d1,%d3
        beq.s execute
        bra.s next_byte
restart_echo:
        moveq #8,%d2
        moveq #0,%d1
        moveq #0,%d5
echo:
        btst #5,UTXH:w
        beq.s echo
        move.b %d6,UTXL:w
        bra.w receive
execute:
        btst #5,UTXH:w
        beq.s execute
        move.b %d6,UTXL:w
drain:
        btst #7,UTXH:w
        beq.s drain
        jmp (%a0)
model_end:
