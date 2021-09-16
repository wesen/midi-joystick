/**# Interrupt driven UART implementation
 **/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "app.h"
#include <util/delay.h>

#include "common.h"

#include "midi_clock.h"

#ifdef MIDI_CLOCK_ENABLE
extern midi_clock_t midi_clock;
#endif

// #define TX_IRQ

/** Ring buffer for incoming data. **/
static uart_rb_t uart_rx_rb;
static uart_rb_t uart_tx_rb;

void uart_rb_init(uart_rb_t *rb) {
  rb->rd = 0;
  rb->wr = 0;
  rb->overflow = 0;
}

void uart_init(void) {
  /** Initialise ring buffers. **/
  uart_rb_init(&uart_rx_rb);
  uart_rb_init(&uart_tx_rb);

  /** Set the baudrate of the serial interface. **/
  UBRRH = (UART_BAUDRATE_REG >> 8);
  UBRRL = (UART_BAUDRATE_REG & 0xFF);
  //  UBRRH = 0;
  //  UBRRL = 15;

  /** 8 bit, no parity **/
  UCSRC = _BV(UCSZ0) | _BV(UCSZ1) | _BV(URSEL);
  /** enable receive, transmit and receive and transmit interrupts. **/
  //  UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
  UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
#ifdef TX_IRQ
  UCSRB |= _BV(TXCIE);
#endif
}

uint8_t uart_sending(void) {
  return !(uart_tx_rb.wr == uart_tx_rb.rd && UART_CHECK_EMPTY_BUFFER());
}


void uart_puts(uint8_t *data, uint8_t cnt) {
  while (cnt--)
    uart_putc(*(data++));
}
/**
 ** Send a character on the serial interface.
 **/
void uart_putc(uint8_t c) {
#ifdef TX_IRQ
  if (uart_tx_rb.wr == uart_tx_rb.rd && UART_CHECK_EMPTY_BUFFER()) {
    UART_WRITE_CHAR(c);
  } else {
    if (UART_RB_INC(uart_tx_rb.wr) == uart_tx_rb.rd) {
      /**
       ** the receive buffer has overflown, read the char directly and discard it.
       **/
      uart_tx_rb.overflow++;
      while (!UART_CHECK_EMPTY_BUFFER())
	;
      /** The buffer is empty, send the character directly. **/
      UART_WRITE_CHAR(c);
      return;
    } else {
      uart_tx_rb.buf[uart_tx_rb.wr] = c;
      uart_tx_rb.wr = UART_RB_INC(uart_tx_rb.wr);
    }
  }
#else
  while (!UART_CHECK_EMPTY_BUFFER())
    ;
  /** The buffer is empty, send the character directly. **/
  UART_WRITE_CHAR(c);
  return;
#endif
}

/**
 ** Check if the receive ring buffer has place left.
 **/
uint8_t uart_avail(void) {
  return (uart_rx_rb.rd != uart_rx_rb.wr);
}

void uart_flush(void) {
  while (!UART_CHECK_EMPTY_BUFFER()) {
    _delay_ms(10);
  }
}

/**
 ** Get a character form the receive ring buffer.
 **/
uint8_t uart_getc(void) {
  /** uart_getc should never be called when tere is no character
   ** available, but we still return 0.
   **/
  
  if (!uart_avail()) {
    return 0;
  }

  uint8_t ret = uart_rx_rb.buf[uart_rx_rb.rd];
  uart_rx_rb.rd = UART_RB_INC(uart_rx_rb.rd);
  return ret;
}

/**
 ** Receive a character from the serial interface and put it into the ring buffer.
 **/
SIGNAL(USART_RXC_vect) {
  if (UART_RB_INC(uart_rx_rb.wr) == uart_rx_rb.rd) {
    /**
     ** the receive buffer has overflown, read the char directly and discard it.
     **/
    uart_rx_rb.overflow++;
    volatile uint8_t a = UART_READ_CHAR();
    a = 0;
    return;
  }

  uint8_t c = UART_READ_CHAR();
#ifdef MIDI_CLOCK_ENABLE
  if (c == 0xF8) {
    midi_clock_handle_midi_clock(&midi_clock);
    return;
  }
#endif
  uart_rx_rb.buf[uart_rx_rb.wr] = c;
  uart_rx_rb.wr = UART_RB_INC(uart_rx_rb.wr);
}

SIGNAL(USART_TXC_vect) {
  if (uart_tx_rb.rd != uart_tx_rb.wr) {
    UART_WRITE_CHAR(uart_tx_rb.buf[uart_tx_rb.rd]);
    uart_tx_rb.rd = UART_RB_INC(uart_tx_rb.rd);
  }
}
