void bootloader_start(void);

__attribute__((section(".bootstart"))) void bootstart(void) {
  bootloader_start();
}
