#ifndef MONOMACHINE_H__
#define MONOMACHINE_H__

#define ELEKTRON_MNM_KIT_DUMP        0x52
#define ELEKTRON_MNM_REQUEST_KIT     0x53
#define ELEKTRON_MNM_LOAD_KIT        0x58
#define ELEKTRON_MNM_ASSIGN_MACHINE  0x5B
#define ELEKTRON_MNM_STATUS_REQUEST  0x70
#define ELEKTRON_MNM_SET_STATUS      0x71
#define ELEKTRON_MNM_STATUS_RESPONSE 0x72

#define ELEKTRON_MNM_REQ_CURRENT_KIT         0x02
#define ELEKTRON_MNM_REQ_CURRENT_PATTERN     0x04
#define ELEKTRON_MNM_REQ_CURRENT_SEQ_MODE    0x21
#define ELEKTRON_MNM_REQ_CURRENT_AUDIO_TRACK 0x22
#define ELEKTRON_MNM_REQ_CURRENT_MIDI_TRACK  0x23

extern uint8_t elektron_sysex_hdr[5];
typedef enum mono_status_e {
  mono_none = 0,
  
  reload_kit_get_current_kit,
  reload_kit_wait_current_kit,
  reload_kit_got_current_kit,

  reload_track_get_current_kit,
  reload_track_wait_current_kit,
  reload_track_got_current_kit,
  reload_track_wait_current_track,
  reload_track_got_current_track,
  reload_track_get_track_data,
  reload_track_wait_track_data,
  reload_track_got_track_data,
  
} mono_status_t;

void mono_routine(void);
void elektron_send_request(uint8_t byte1, uint8_t byte2);
void handle_elektron_sysex(uint8_t c);
void start_elektron_sysex(void);
void end_elektron_sysex(void);

uint16_t sysex_cnt;
extern uint8_t buflen;
extern uint8_t buf_record;
extern uint8_t buf[128];
extern uint8_t sysex_discard;

void sysex_record(void);

extern volatile mono_status_t mono_status;
extern uint8_t reload_track;
#endif /* MONOMACHINE_H__ */
