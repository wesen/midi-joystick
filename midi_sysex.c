#include "common.h"

#include "midi.h"

void sysex_in_stm_reset(sysex_in_stm_t *stm) {
  stm->address = 0;
  stm->cmd = 0;
  stm->length = 0;
  stm->checksum = 0;
  stm->state = sysex_wait_start;
  stm->recvd = 0;
}

void sysex_in_stm_init(sysex_in_stm_t *stm) {
  sysex_in_stm_reset(stm);
  stm->callback = NULL;
}

void handle_midi_rx_only_sysex(midi_in_stm_t *stm, uint8_t byte) {
 again:
  if (MIDI_IS_REALTIME_STATUS_BYTE(byte))
    return;

  switch (stm->in_state) {
  case midi_ignore_message:
    stm->in_state = midi_wait_status;
    goto again;
    
  case midi_wait_sysex:
    if (MIDI_IS_STATUS_BYTE(byte) && (byte != MIDI_SYSEX_END)) {
      /* status byte interrupts sysex request */
      stm->in_state = midi_wait_status;
      midi_abort_sysex(stm);
      goto again;
    }
    midi_byte_sysex(stm, byte);
    break;
    
  case midi_wait_status:
    {
      /* special handling for sysex */
      if (byte == MIDI_SYSEX_START) {
	stm->in_state = midi_wait_sysex;
	midi_start_sysex(stm);
	stm->running_status = 0;
	stm->last_status = 0;
	return;
      }
    }
    break;

  default:
    stm->in_state = midi_wait_status;
    goto again;
    break;
  }
}

void midi_start_sysex(midi_in_stm_t *stm) {
  sysex_in_stm_reset(&stm->sysex_stm);
  stm->sysex_stm.state = sysex_wait_vendor_1;
#ifdef __APPLE__
  printf("midi start sysex\n");
#endif
}

void midi_abort_sysex(midi_in_stm_t *stm) {
  stm->sysex_stm.state = sysex_wait_start;
}

void midi_end_sysex(midi_in_stm_t *stm) {
  if (stm->sysex_stm.callback != NULL)
    stm->sysex_stm.callback(&stm->sysex_stm);
  stm->sysex_stm.state = sysex_wait_start;
}

void midi_byte_sysex(midi_in_stm_t *stm, unsigned char byte) {
  sysex_in_stm_t *sstm = &stm->sysex_stm;

  //  printf("stm state %d, byte %x, recvd %d\n", sstm->state, byte, sstm->recvd);
  if ((byte == 0xF7)) {
    if (sstm->state == sysex_wait_end)
      midi_end_sysex(stm);
    else
      midi_abort_sysex(stm);
    sstm->state = sysex_wait_start;
    return;
  }
  
  switch (sstm->state) {
  case sysex_wait_start:
    /* nothing to do */
    break;

  case sysex_wait_vendor_1:
    if (byte != SYSEX_VENDOR_1)
      sstm->state = sysex_wait_start;
    else {
      if (byte == 0x00)
	sstm->state = sysex_wait_vendor_2;
      else
	sstm->state = sysex_wait_cmd;
    }
    break;

  case sysex_wait_vendor_2:
    if (byte != SYSEX_VENDOR_2)
      sstm->state = sysex_wait_start;
    else
      sstm->state = sysex_wait_vendor_3;
    break;

  case sysex_wait_vendor_3:
    if (byte != SYSEX_VENDOR_3)
      sstm->state = sysex_wait_start;
    else
      sstm->state = sysex_wait_cmd;
    break;

  case sysex_wait_cmd:
    sstm->cmd = byte;
    sstm->checksum ^= byte;
    switch (sstm->cmd) {
    case CMD_DATA_BLOCK_ACK:
    case CMD_MAIN_PROGRAM:
    case CMD_START_BOOTLOADER:
      sstm->state = sysex_wait_end;
      break;

#ifdef SYSEX_PATTERNS
    case CMD_WRITE_PATTERN:
      sstm->length = 35;
      sstm->state = sysex_wait_data;
      break;

    case CMD_READ_PATTERN:
      sstm->state = sysex_wait_patnum;
      break;

    case CMD_SWITCH_PATTERN:
      sstm->state = sysex_wait_patnum;
      break;
#endif

#ifdef BOOTLOADER
    case CMD_BOOT_DATA_BLOCK:
      sstm->state = sysex_wait_length;
      break;

    case CMD_FIRMWARE_CHECKSUM:
      sstm->state = sysex_wait_len1;
      break;
#endif

    default:
      sstm->state = sysex_wait_start;
      break;
    }
    break;

#ifdef SYSEX_PATTERNS
  case sysex_wait_patnum:
    sstm->address = byte;
    sstm->state = sysex_wait_end;
    break;
#endif

#ifdef BOOTLOADER
  case sysex_wait_len1:
    sstm->data[0] = byte;
    sstm->state = sysex_wait_len2;
    break;

  case sysex_wait_len2:
    sstm->data[1] = byte;
    sstm->state = sysex_wait_len3;
    break;

  case sysex_wait_len3:
    sstm->data[2] = byte;
    sstm->state = sysex_wait_checksum1;
    break;

  case sysex_wait_checksum1:
    sstm->data[3] = byte;
    sstm->state = sysex_wait_checksum2;
    break;

  case sysex_wait_checksum2:
    sstm->data[4] = byte;
    sstm->state = sysex_wait_end;
    break;

  case sysex_wait_length:
    sstm->length = byte;
    sstm->state = sysex_wait_addr1;
    sstm->checksum ^= byte;
    sstm->cnt = 0;
    break;

  case sysex_wait_addr1:
    sstm->checksum ^= byte;
    sstm->address = byte;
    sstm->state = sysex_wait_addr2;
    break;
    
  case sysex_wait_addr2:
    sstm->checksum ^= byte;
    sstm->address |= ((uint32_t)byte << 7);
    sstm->state = sysex_wait_addr3;
    break;
    
  case sysex_wait_addr3:
    sstm->checksum ^= byte;
    sstm->address |= ((uint32_t)byte << 14);
    sstm->state = sysex_wait_addr4;
    break;
    
  case sysex_wait_addr4:
    sstm->checksum ^= byte;
    sstm->address |= ((uint32_t)byte << 21);
    sstm->state = sysex_wait_data;
    break;

  case sysex_wait_data:
    sstm->checksum ^= byte;
    if ((sstm->cnt % 8) == 0) {
      sstm->bits = byte;
    } else {
      sstm->data[sstm->recvd++] = byte | ((sstm->bits & 1) << 7);
      sstm->bits >>= 1;
    }
    if (sstm->recvd >= sstm->length) {
      sstm->state = sysex_wait_checksum;
    }
    sstm->cnt++;
    break;

  case sysex_wait_checksum:
#ifdef __APPLE__
    printf("checksum %x, recv %x\n", sstm->checksum & 0x7F, byte & 0x7F);
#endif
    sstm->checksum &= 0x7f;
    if (sstm->checksum != byte) {
      sstm->state = sysex_wait_start;
      break;
    } else {
      sstm->state = sysex_wait_end;
    }
    break;
#endif

  case sysex_wait_end:
  default:
    midi_abort_sysex(stm);
    sstm->state = sysex_wait_start;
    break;
  }
}
