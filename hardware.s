.include "common.inc"

NUM_BUTTONS = 10

.global clear_buttons
.global handle_buttons

.text

        .section	.text.handle_buttons,"ax",@progbits
handle_buttons:
	;; r24 = but_tmp
	push r16
	push r17
	push r28
	push r29
	
	mov  r17, r24 		; r17 = buttons

	ldi  r16, 0 		; r16 = i
;; 	ldi  r30, lo8(buttons)	; Z = buttons
;; 	ldi  r31, hi8(buttons)
	movw r30, r22

button_loop:
	ld   r18, Z             ; r18 = buttons[i].status

	andi r18, 0xFE
	sbrc r17, 0		; lower bit of r18 = lower bit of r17
	ori  r18, 1

	sbrs r18, 1		; if (BUTTON_PRESSED(i))
	rjmp .L1
	sbrc r18, 0
	rjmp .L1

	;; button_pressed(i) == 1

	;; B_PRESS_TIME(i) = timer2_slowclock
	lds  r24, timer2_slowclock 
	lds  r25, timer2_slowclock+1 
	std  Z+1, r24
	std  Z+2, r25

	sbrs r18, 2		; B_PRESSED_ONCE(i)
	rjmp not_pressed_once

	;; B_PRESSED_ONCE(i)
	ldd  r22, Z+1
	ldd  r23, Z+2
	ldd  r24, Z+3
	ldd  r25, Z+4
	rcall clock_diff 	; clock_diff(B_LAST_PRESS_TIME(i), B_PRESS_TIME(i))

	subi r24, lo8(300)
	sbci r25, hi8(300)
	brsh .L1

	;; diff < DOUBLE_CLICK_TIME
	ori  r18, 8		; SET_B_DOUBLE_CLICK(i)
	andi r18, -5 		; CLEAR_B_PRESSED_ONCE(i)
	rjmp .L1

	;; !B_PRESSED_ONCE(i)
not_pressed_once:
	std  Z+3, r24
	std  Z+4, r25 		; B_LAST_PRESS_TIME(i) = B_PRESS_TIME(i)
	ori  r18, 4 		; SET_B_PRESSED_ONCE

.L1:
	sbrc r18, 0		; BUTTON_DOWN(i)
	rjmp .L2
	sbrs r18, 2		; B_PRESSED_ONCE(i)
	rjmp .L2

	;; BUTTON_DOWN(i) && B_PRESSED_ONCE(i)
	lds  r22, timer2_slowclock
	lds  r23, timer2_slowclock+1
	ldd  r24, Z+3
	ldd  r25, Z+4
	rcall clock_diff	; clock_diff(B_LAST_PRESS_TIME(i), timer2_slowclock)

	subi r24, lo8(1001)
	sbci r25, hi8(1001)
	brlo .L2

	;;  diff > LONG_CLICK_TIME
	ori  r18, 32 		; SET_B_LONG_CLICK(i)
	andi r18, -5		; CLEAR_B_PRESSED_ONCE(i)

.L2:
	sbrs r18, 0		; BUTTON_UP(i)
	rjmp .L3
	sbrs r18, 2		; B_PRESSED_ONCE(i)
	rjmp .L3

	lds  r22, timer2_slowclock
	lds  r23, timer2_slowclock+1
	ldd  r24, Z+3
	ldd  r25, Z+4
	rcall clock_diff	; clock_diff(B_LAST_PRESS_TIME(i), timer2_slowclock)

	ldi  r19, hi8(1001)
	cpi  r24, lo8(1001)
	cpc  r25, r19
	brlo short_diff

	;; diff > LONG_CLICK_TIME
	andi r18, -5		; CLEAR_B_PRESSED_ONCE(i)
	rjmp .L3

short_diff:
	subi r24, lo8(301)
	sbci r25, hi8(301)
	brlo .L3

	;; diff > DOUBLE_CLICK_TIME
	andi r18, -5		; CLEAR_B_PRESSED_ONCE(i)
	ori  r18, 16		; SET_B_CLICK(i)

.L3:
	st   Z, r18		; store status back
	
	adiw r30, 5		; Z += sizeof(button_t)

	lsr  r17 		; but_tmp >>= 1
	
	subi r16, -1		; i++ < 6
;; 	cpi  r16, NUM_BUTTONS
	cp   r16, r20
	brne 1f
	rjmp handle_ret
1:	
	rjmp button_loop

handle_ret:
	pop r29
	pop r28
	pop r17
	pop r16
	ret

        .section	.text.clear_buttons,"ax",@progbits
clear_buttons:
	ldi  r30, lo8(buttons)
	ldi  r31, hi8(buttons)
	clr  r19

clear_loop:
	ld   r18, Z
	andi r18, -9 		; CLEAR_B_DOUBLE_CLICK(i)
	andi r18, -17		; CLEAR_B_CLICK
	andi r18, -33 		; CLEAR_B_LONG_CLICK

1:	
	andi r18, -3		; STORE_B_OLD(i, B_CURRENT(i))
	sbrc r18, 0
	ori  r18, 2

	st   Z, r18
	adiw r30, 5

	subi r19, -1
	cpi  r19, NUM_BUTTONS
	brne clear_loop
	
	ret

