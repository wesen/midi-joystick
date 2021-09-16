__SREG__ = 0x3f
__SP_H__ = 0x3e
__SP_L__ = 0x3d
__tmp_reg__ = 0
__zero_reg__ = 1
	.global __do_copy_data
	.global __do_clear_bss

	.text
	.section	.text.check_firmware_checksum,"ax",@progbits
.global	check_firmware_checksum
	.type	check_firmware_checksum, @function
check_firmware_checksum:
	ldi r26,lo8(0)	 
	ldi r27,hi8(0)	 
	rcall __eeprom_read_word_1C1D1E
	
	movw r20,r30	 
	ldi r26,lo8(2)	 
	ldi r27,hi8(2)	 
	rcall __eeprom_read_word_1C1D1E
	
	movw r22,r30	 
	ldi r30,lo8(0)	 
	ldi r31,hi8(0)	 
	movw r18, r30
	rjmp .L2	 
.L3:
	lpm r24, Z	 
	
	add r18,r24	 
	adc r19,__zero_reg__	 
	adiw r30,1	 
.L2:
	cp r30,r20	 
	cpc r31,r21	 
	brne .L3	 
	ldi r24,lo8(0)	 
	andi r19,hi8(16383)	 
	cp r18,r22	 
	cpc r19,r23	 
	brne .L5	 
	ldi r24,lo8(1)	 
.L5:
	ret
	.size	check_firmware_checksum, .-check_firmware_checksum

	.section	.text.write_firmware_checksum,"ax",@progbits
.global	write_firmware_checksum
	.type	write_firmware_checksum, @function
write_firmware_checksum:
	ldi r26,lo8(0)	 
	ldi r27,hi8(0)	 
	movw __tmp_reg__,r24	 
	rcall __eeprom_write_word_1C1D1E
	
	ldi r26,lo8(2)	 
	ldi r27,hi8(2)	 
	movw __tmp_reg__,r22	 
	rcall __eeprom_write_word_1C1D1E
	
	ldi r24,lo8(1)	 
	ldi r25,hi8(1)	 
	ldi r26,lo8(4)	 
	ldi r27,hi8(4)	 
	movw __tmp_reg__,r24	 
	rcall __eeprom_write_word_1C1D1E
	ret

	.size	write_firmware_checksum, .-write_firmware_checksum
	.section	.text.bl_uart_getc,"ax",@progbits
.global	bl_uart_getc
	.type	bl_uart_getc, @function
bl_uart_getc:
.L11:
	sbis 43-0x20,7	 
	rjmp .L11	 
	in r24,44-0x20	 
	clr r25	 
	ret

	.size	bl_uart_getc, .-bl_uart_getc
	.section	.text.bl_uart_putc,"ax",@progbits
.global	bl_uart_putc
	.type	bl_uart_putc, @function
bl_uart_putc:


.L18:
	sbis 43-0x20,5	 
	rjmp .L18	 
	out 44-0x20,r24	 
	ret

	.size	bl_uart_putc, .-bl_uart_putc
	.section	.text.midi_sysex_send_ack,"ax",@progbits
.global	midi_sysex_send_ack
	.type	midi_sysex_send_ack, @function
midi_sysex_send_ack:


	ldi r30,lo8(ack_msg)	 
	ldi r31,hi8(ack_msg)	 
.L24:
	ld r24,Z	 
.L25:
	sbis 43-0x20,5	 
	rjmp .L25	 
	out 44-0x20,r24	 
	adiw r30,1	 
	ldi r24,hi8(ack_msg+6)	 
	cpi r30,lo8(ack_msg+6)	 
	cpc r31,r24	 
	brne .L24	 
	ret
	.size	midi_sysex_send_ack, .-midi_sysex_send_ack

	.section	.text.midi_sysex_send_nak,"ax",@progbits
.global	midi_sysex_send_nak
	.type	midi_sysex_send_nak, @function
midi_sysex_send_nak:
	ldi r30,lo8(nak_msg)	 
	ldi r31,hi8(nak_msg)	 
.L34:
	ld r24,Z	 
.L35:
	sbis 43-0x20,5	 
	rjmp .L35	 
	out 44-0x20,r24	 
	adiw r30,1	 
	ldi r24,hi8(nak_msg+6)	 
	cpi r30,lo8(nak_msg+6)	 
	cpc r31,r24	 
	brne .L34	 
	ret
	.size	midi_sysex_send_nak, .-midi_sysex_send_nak

	.section	.text.jump_to_main_program,"ax",@progbits
.global	jump_to_main_program
	.type	jump_to_main_program, @function
jump_to_main_program:
	rcall check_firmware_checksum	 
	tst r24	 
	brne .L44	 
	ldi r24,lo8(0)	 
	ret
.L44:
	lds r30, jump_to_app
	lds r31, jump_to_app+1
	icall
	.size	jump_to_main_program, .-jump_to_main_program

	.section	.text.make_word,"ax",@progbits
.global	make_word
	.type	make_word, @function
make_word:
	mov r23,r24	 
	subi r23,lo8(-(-1))	 
	add r22,r23	 
	subi r23,lo8(-(1))	 
	ldi r18,lo8(0)	 
	ldi r19,hi8(0)	 
	ldi r20,hlo8(0)	 
	ldi r21,hhi8(0)	 
	rjmp .L49	 
.L50:
	movw r26,r20	 
	movw r24,r18	 
	ldi r18,7	 
1:	lsl r24	 
	rol r25	 
	rol r26	 
	rol r27	 
	dec r18	 
	brne 1b
	mov r30,r22	 
	clr r31	 
	subi r30,lo8(-(data))	 
	sbci r31,hi8(-(data))	 
	ld r18,Z	 
	clr r19	 
	clr r20	 
	clr r21	 
	or r18,r24	 
	or r19,r25	 
	or r20,r26	 
	or r21,r27	 
	subi r22,lo8(-(-1))	 
.L49:
	cp r22,r23	 
	brsh .L50	 
	movw r24,r18	 
	ret
	.size	make_word, .-make_word

	.section	.text.write_checksum,"ax",@progbits
.global	write_checksum
	.type	write_checksum, @function
write_checksum:


	ldi r22,lo8(3)	 
	ldi r24,lo8(4)	 
	rcall make_word	 
	lds r20,data+8	 
	clr r21	 
	lsr r21	 
	mov r21,r20	 
	clr r20	 
	ror r21	 
	ror r20	 
	lds r18,data+7	 
	clr r19	 
	or r20,r18	 
	or r21,r19	 
	ldi r26,lo8(0)	 
	ldi r27,hi8(0)	 
	movw __tmp_reg__,r24	 
	rcall __eeprom_write_word_1C1D1E
	
	ldi r26,lo8(2)	 
	ldi r27,hi8(2)	 
	movw __tmp_reg__,r20	 
	rcall __eeprom_write_word_1C1D1E
	
	ldi r24,lo8(1)	 
	ldi r25,hi8(1)	 
	ldi r26,lo8(4)	 
	ldi r27,hi8(4)	 
	movw __tmp_reg__,r24	 
	rcall __eeprom_write_word_1C1D1E
	
	ret
	.size	write_checksum, .-write_checksum

	.section	.text.write_block,"ax",@progbits
.global	write_block
	.type	write_block, @function
write_block:

	push r14
	push r15
	push r16
	push r17
	push r28
	push r29

	lds r24,sysex_cnt	 
	mov r18,r24	 
	clr r19	 
	subi r18,lo8(-(-1))	 
	sbci r19,hi8(-(-1))	 
	clr r15	 
	ldi r25,lo8(3)	 
	rjmp .L56	 
.L57:
	subi r30,lo8(-(data))	 
	sbci r31,hi8(-(data))	 
	ld r24,Z	 
	eor r15,r24	 
	subi r25,lo8(-(1))	 
.L56:
	mov r30,r25	 
	clr r31	 
	cp r30,r18	 
	cpc r31,r19	 
	brlt .L57	 
	lds r14,data+4	 
	ldi r22,lo8(4)	 
	ldi r24,lo8(5)	 
	rcall make_word	 
	movw r16,r24	 
	ldi r24,hi8(7168)	 
	cpi r16,lo8(7168)	 
	cpc r17,r24	 
	brlo .+2	 
	rjmp .L59
	
	lds r24,sysex_cnt	 
	mov r26,r24	 
	clr r27	 
	movw r22,r26	 
	subi r22,lo8(-(-9))	 
	sbci r23,hi8(-(-9))	 
	ldi r21,lo8(0)	 
	ldi r20,lo8(0)	 
	ldi r18,lo8(0)	 
	ldi r19,hi8(0)	 
	rjmp .L61	 
.L62:
	movw r28,r24	 
	subi r28,lo8(-(data))	 
	sbci r29,hi8(-(data))	 
	andi r24,lo8(7)	 
	andi r25,hi8(7)	 
	or r24,r25	 
	brne .L63	 
	ldd r20,Y+9	 
	rjmp .L65	 
.L63:
	mov r30,r21	 
	clr r31	 
	subi r30,lo8(-(sysex_data))	 
	sbci r31,hi8(-(sysex_data))	 
	mov r24,r20	 
	ror r24	 
	clr r24	 
	ror r24	 
	ldd r25,Y+9
	or r24,r25	 
	st Z,r24	 
	subi r21,lo8(-(1))	 
	lsr r20	 
.L65:
	subi r18,lo8(-(1))	 
	sbci r19,hi8(-(1))	 
	cp r21,r14	 
	brsh .L66	 
.L61:
	mov r24,r18	 
	clr r25	 
	cp r24,r22	 
	cpc r25,r23	 
	brlt .L62	 
.L66:
	ldi r30,lo8(127)	 
	and r15,r30	 
	subi r26,lo8(-(data-1))	 
	sbci r27,hi8(-(data-1))	 
	ld r24,X	 
	cp r15,r24	 
	breq .+2	 
	rjmp .L59	 
	cpi r21,lo8(64)	 
	breq .+2	 
	rjmp .L59	 
	in r25,95-0x20	 
	cli
	ldi r24,lo8(3)	 
	movw r30, r16	 
	sts 87, r24	 
	spm
	
.L69:
	in __tmp_reg__,87-0x20	 
	sbrc __tmp_reg__,0	 
	rjmp .L69	 
	ldi r26,lo8(sysex_data)	 
	ldi r27,hi8(sysex_data)	 
	movw r22,r16	 
.L71:
	ldi r24,lo8(1)	 
	movw r30,r26	 
	ldd r18,Z+1	 
	clr r19	 
	mov r19,r18	 
	clr r18	 
	ld r20,X	 
	clr r21	 
	or r18,r20	 
	or r19,r21	 
	movw  r0, r18	 
	movw r30, r22	 
	sts 87, r24	 
	spm
	clr  r1
	
	subi r22,lo8(-(2))	 
	sbci r23,hi8(-(2))	 
	adiw r26,2	 
	ldi r31,hi8(sysex_data+64)	 
	cpi r26,lo8(sysex_data+64)	 
	cpc r27,r31	 
	brne .L71	 
	ldi r24,lo8(5)	 
	movw r30, r16	 
	sts 87, r24	 
	spm
	
.L73:
	in __tmp_reg__,87-0x20	 
	sbrc __tmp_reg__,0	 
	rjmp .L73	 
	ldi r24,lo8(17)	 
	sts 87, r24	 
	spm
	
	out 95-0x20,r25	 
	ldi r24,lo8(1)	 
	rjmp .L75	 
.L59:
	ldi r24,lo8(0)	 
.L75:
	ldi r30,6
	in r28,__SP_L__
	in r29,__SP_H__
	rjmp __epilogue_restores__+24
	.size	write_block, .-write_block

	.section	.text.handle_sysex,"ax",@progbits
.global	handle_sysex
	.type	handle_sysex, @function
handle_sysex:
	lds r25,sysex_cnt	 
	cpi r25,lo8(4)	 
	brlo .L96
	
	lds r24,data+3	 
	cpi r24,lo8(5)	 
	breq .L86	 
	cpi r24,lo8(4)	 
	brne .L88	 
	rcall jump_to_main_program	 
	rjmp .L90	 
.L88:
	cpi r24,lo8(1)	 
	brne .L91	 
	rcall write_block	 
	rjmp .L90	 
.L91:
	cpi r24,lo8(3)	 
	brne .L93	 
	cpi r25,lo8(9)	 
	brne .L93	 
	rcall write_checksum	 
.L90:
	tst r24	 
	breq .L93	 
.L86:
	rcall midi_sysex_send_ack	 
	ret
.L93:
	rcall midi_sysex_send_nak	 
.L96:
	ret
	.size	handle_sysex, .-handle_sysex

	.section	.text.handle_midi,"ax",@progbits
.global	handle_midi
	.type	handle_midi, @function
handle_midi:

	mov r25,r24
	lds r24,in_sysex	 

	;; compare to 0xF0
	cpi r25,lo8(-16)	 
	brne .L98	 
	cpi r24,lo8(1)	 
	brne .L100	 
	rcall handle_sysex	 
.L100:
	sts sysex_cnt,__zero_reg__	 
	ldi r24,lo8(1)	 
	sts in_sysex,r24	 
	ret

	;; compare to 0xF7
.L98:
	cpi r25,lo8(-9)	 
	brne .L103	 
	cpi r24,lo8(1)	 
	brne .L117	 
	rcall handle_sysex	 
	rjmp .L117

	;;  status byte
.L103:
	sbrs r24,7	 
	rjmp .L107	 
.L117:
	sts in_sysex,__zero_reg__	 
	ret
.L107:
	tst r24	 
	breq .L116	 
	lds r24,sysex_cnt	 
	mov r30,r24	 
	clr r31	 
	subi r30,lo8(-(data))	 
	sbci r31,hi8(-(data))	 
	st Z,r25	 
	mov r25,r24	 	
	subi r25,lo8(-(1))	 
	sts sysex_cnt,r25	 
	cpi r25,lo8(3)	 
	brlo .L110	 
	lds r24,data	 
	tst r24	 
	brne .L112	 
	lds r24,data+1	 
	cpi r24,lo8(19)	 
	brne .L112	 
	lds r24,data+2	 
	cpi r24,lo8(55)	 
	breq .L110	 
.L112:
	sts in_sysex,__zero_reg__	 
.L110:
	cpi r25,lo8(101)	 
	brlo .L116	 
	sts in_sysex,__zero_reg__	 
.L116:
	ret
	.size	handle_midi, .-handle_midi

	.section	.text.main,"ax",@progbits
.global	main
	.type	main, @function
main:
	ldi r28,lo8(__stack - 0)
	ldi r29,hi8(__stack - 0)
	out __SP_H__,r29
	out __SP_L__,r28

	ldi r24,lo8(24)	 
	in __tmp_reg__, __SREG__
	cli
	out 33, r24	 
	out 33, __zero_reg__	 
	out __SREG__,__tmp_reg__
	
	ldi r24,lo8(3)	 
	out 91-0x20,r24	 
	cbi 56-0x20,4	 
	sbi 55-0x20,0	 
	ldi r26,lo8(4)	 
	ldi r27,hi8(4)	 
	rcall __eeprom_read_word_1C1D1E
	
	sbiw r30,1	 
	brne .L119	 
	sbic 54-0x20,4	 
	rcall jump_to_main_program	 
.L119:
	out 64-0x20,__zero_reg__	 
	ldi r24,lo8(31)	 
	out 41-0x20,r24	 
	ldi r24,lo8(-122)	 
	out 64-0x20,r24	 
	ldi r24,lo8(24)	 
	out 42-0x20,r24	 
	rcall midi_sysex_send_ack	 
	ldi r17,lo8(0)	 
.L136:
	sbis 54-0x20,4	 
	rjmp .L123	 
	ldi r17,lo8(1)	 
	rjmp .L125	 
.L123:
	cpse r17,__zero_reg__	 
	rcall jump_to_main_program	 
.L125:
	sbis 43-0x20,7	 
	rjmp .L136	 
	in r24,44-0x20	 
	rcall handle_midi	 
	rjmp .L136	 
	.size	main, .-main

.global	jump_to_app
.global	jump_to_app
	.section .bss
	.type	jump_to_app, @object
	.size	jump_to_app, 2
jump_to_app:
	.skip 2,0
.global	ack_msg
	.data
	.type	ack_msg, @object
	.size	ack_msg, 6
ack_msg:
	.byte	-16
	.byte	0
	.byte	19
	.byte	55
	.byte	2
	.byte	-9
.global	nak_msg
	.type	nak_msg, @object
	.size	nak_msg, 6
nak_msg:
	.byte	-16
	.byte	0
	.byte	19
	.byte	55
	.byte	16
	.byte	-9
.global	block_cnt
.global	block_cnt
	.section .bss
	.type	block_cnt, @object
	.size	block_cnt, 1
block_cnt:
	.skip 1,0
.global	in_sysex
.global	in_sysex
	.type	in_sysex, @object
	.size	in_sysex, 1
in_sysex:
	.skip 1,0
	.comm sysex_data,64,1
	.comm data,100,1
	.comm sysex_cnt,1,1
	.text
.Letext0:
