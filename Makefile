CC=avr-gcc
OBJCOPY = avr-objcopy
UISP=uisp
AVR_ARCH = atmega8
LDAVR_ARCH = avrmega8
CFLAGS += -Os -ffunction-sections -DAVR -I. -mmcu=$(AVR_ARCH) -mcall-prologues -fshort-enums -fpack-struct
CFLAGS += -Wall -DLITTLE_ENDIAN -g
CLDFLAGS += -Wl,--gc-sections -ffunction-sections
CLDFLAGS += -mmcu=$(AVR_ARCH)
LDFLAGS = -m $(LDAVR_ARCH) -M
UISP_TARGET=-dprog=stk200 -dlpt=0x3bc -dpart=atmega8

PROJ=midi-joystick

# atmega 8 1024 word boot size = 0x1c00 * 2 = 0x1800
BOOTLOADER_START = 0x1800

#CLDFLAGS += -Wl,--section-start=.bootstart=$(BOOTLOADER_START)
#CLDFLAGS += -Wl,--section-start=.bootloader=0x1820
#CLDFLAGS += -Wl,--section-start=.bootloader.formidable=0x0880
#CLDFLAGS += -Wl,--section-start=.data=0x800

all: $(PROJ).hex bootloader.hex 

firmware.zip: midi-joystick.syx 
	rm -rf /tmp/mono-joystick-firmware
	mkdir /tmp/mono-joystick-firmware
	cp README.FIRMWARE /tmp/mono-joystick-firmware/README-FIRMWARE.txt
	cp midi-joystick.syx /tmp/mono-joystick-firmware
	cd /tmp && zip -r mono-joystick-firmware.zip mono-joystick-firmware && scp mono-joystick-firmware.zip mnl@jockel.hasis.biz:rw-support


%.o: %.c Makefile
	$(CC) $(CFLAGS) -Wa,-adhlns=$@.lst -c $< -o $@


%.o: %.s Makefile
	$(CC) $(CFLAGS) -Wa,-adhlns=$@.lst -c $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) -fverbose-asm $< -o $@

%.d:%.c
	set -e; $(CC) -MM $(CFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@ ; \
	[ -s $@ ] || rm -f $@

%.syx: %.hex
	ihex2sysex $< $@

# CONVERT ELF
%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.ee_srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@

midi-joystick.elf: uart.o midi-small.o \
		 sr165.o main.o \
		common.o hardware.o monomachine.o
	$(CC) $(CLDFLAGS) -o $@ $+
	avr-size $@
	./left.sh $@

midi_sysex_bootloader.o: midi_sysex.c
	$(CC) $(CFLAGS) -c -DBOOTLOADER $< -o $@

empty.elf: empty-main.o
	$(CC) $(CLDFLAGS) -o $@ $+

bootloader.elf: bootloader.o 
	$(CC) $(CLDFLAGS) -Wl,--section-start=.text=0x1800 -o $@ $+

bootloader-asm.elf: bootloader-asm.o 
	$(CC) $(CLDFLAGS) -Wl,--section-start=.text=0x1C00 -o $@ $+
#	$(CC) $(CLDFLAGS) -o $@ $+

# PROGRAM
uisp: $(PROJ).srec # $(PROJ).ee_srec $(PROJ).hex
	$(UISP) --segment=flash $(UISP_TARGET) --erase --upload if=$(PROJ).srec --verify
	$(UISP) --segment=eeprom $(UISP_TARGET) --upload if=$(PROJ).ee_srec --verify

avrdude: $(PROJ).hex bootloader.hex # $(PROJ).ee_srec $(PROJ).hex
	avrdude -p m8 -P usb -c usbasp -U flash:w:$(PROJ).hex

avrread: 
	avrdude -p m8 -P usb -c usbasp -U flash:r:$(PROJ)-read.hex:i


bootavrdude: bootloader.hex # $(PROJ).ee_srec $(PROJ).hex
	avrdude -p m8 -P usb -c usbasp -U flash:w:bootloader.hex

bootasmdude: bootloader-asm.hex # $(PROJ).ee_srec $(PROJ).hex
	avrdude -p m8 -P usb -c usbasp -U flash:w:bootloader-asm.hex

midiupload: clean midi-joystick.syx
	midi-send -b -i1 -o1 midi-joystick.syx


verify: $(PROJ).srec $(PROJ).ee_srec
	$(UISP) --segment=flash $(UISP_TARGET) --verify if=$(PROJ).srec

init:
#       enable watchdog, external crystal
#	$(UISP) $(UISP_TARGET) --wr_fuse_l=0xc0 --wr_fuse_h=0xd9
# 1024 words bootloader
	avrdude -p m8 -P usb -c usbasp -U hfuse:w:0xd8:m -U lfuse:w:0x0f:m
init512:
# 512 words bootloader
	avrdude -p m8 -P usb -c usbasp -U hfuse:w:0xda:m -U lfuse:w:0x0f:m

restart:
#       read the fuses to reset the programming adapter
#	$(UISP) $(UISP_TARGET) --rd_fuses
	avrdude -p m8 -P usb -c usbasp 

clean:
	rm -f *.o *.d *~ $(PROJ) *.srec *.ee_srec *.hex *lst *elf *syx
