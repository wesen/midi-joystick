#ifndef SR165_H__
#define SR165_H__

  
#define SR165_OUT    PB1
#define SR165_SHLOAD PB3
#define SR165_CLK    PB2

#define SR165_DATA_PORT PORTB
#define SR165_DDR_PORT   DDRB
#define SR165_PIN_PORT  PINB

void sr165_init(void);
uint8_t sr165_read(void);
uint8_t sr165_read_norst(void);
uint16_t sr165_read16(void);

#endif /* SR165_H__ */
