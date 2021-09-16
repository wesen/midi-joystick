#ifndef SYSEX_DATA_H__
#define SYSEX_DATA_H__

uint8_t data_to_sysex(uint8_t *data, uint8_t *sysex, uint8_t len);
uint8_t sysex_to_data(uint8_t *sysex, uint8_t *data, uint8_t len);

#endif /* SYSEX_DATA_H__ */
