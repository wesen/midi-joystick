#ifndef KONTROL_SYSEX_H__
#define KONTROL_SYSEX_H__

uint8_t data_to_sysex(uint8_t *data, uint8_t *sysex, uint8_t len);
uint8_t sysex_to_data(uint8_t *sysex, uint8_t *data, uint8_t len);
void send_page_sysex(uint8_t page);
void send_patch_sysex(uint8_t patch);
void receive_page_sysex(void);
void save_page_sysex(void);

#endif /* KONTROL_SYSEX_H__ */
