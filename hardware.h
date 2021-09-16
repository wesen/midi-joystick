#ifndef HARDWARE_H__
#define HARDWARE_H__

#define NUM_BUTTONS  10
#define SHIFT_BUTTON 6
#define SELECT_BUTTON 7
#define MUTE_BUTTON 8
#define JOYSTICK_BUTTON 9

typedef struct button_s {
  uint8_t status;
  uint16_t press_time;
  uint16_t last_press_time;
} button_t;

#define B_BIT_CURRENT      0
#define B_BIT_OLD          1
#define B_BIT_PRESSED_ONCE 2
#define B_BIT_DOUBLE_CLICK 3
#define B_BIT_CLICK        4
#define B_BIT_LONG_CLICK   5

#define DOUBLE_CLICK_TIME 30
#define LONG_CLICK_TIME  100

#define B(i)                   (buttons[(i)])

#define B_STATUS(i, bit)        (IS_BIT_SET8(buttons[i].status, bit))
#define B_SET_STATUS(i, bit)    (SET_BIT8(buttons[i].status, bit))
#define B_CLEAR_STATUS(i, bit)  (CLEAR_BIT8(buttons[i].status, bit))
#define B_STORE_STATUS(i, bit, j) { if (j) B_SET_STATUS(i, bit);	\
      else B_CLEAR_STATUS(i, bit); }

#define B_CURRENT(i)               (B_STATUS(i, B_BIT_CURRENT))
#define SET_B_CURRENT(i)           (B_SET_STATUS(i, B_BIT_CURRENT))
#define CLEAR_B_CURRENT(i)         (B_CLEAR_STATUS(i, B_BIT_CURRENT))
#define STORE_B_CURRENT(i, j)      (B_STORE_STATUS(i, B_BIT_CURRENT, j))

#define B_OLD(i)                   (B_STATUS(i, B_BIT_OLD))
#define SET_B_OLD(i)               (B_SET_STATUS(i, B_BIT_OLD))
#define CLEAR_B_OLD(i)             (B_CLEAR_STATUS(i, B_BIT_OLD))
#define STORE_B_OLD(i, j)          (B_STORE_STATUS(i, B_BIT_OLD, j))

#define B_PRESSED_ONCE(i)          (B_STATUS(i, B_BIT_PRESSED_ONCE))
#define SET_B_PRESSED_ONCE(i)      (B_SET_STATUS(i, B_BIT_PRESSED_ONCE))
#define CLEAR_B_PRESSED_ONCE(i)    (B_CLEAR_STATUS(i, B_BIT_PRESSED_ONCE))

#define B_CLICK(i)                 (B_STATUS(i, B_BIT_CLICK))
#define SET_B_CLICK(i)             (B_SET_STATUS(i, B_BIT_CLICK))
#define CLEAR_B_CLICK(i)           (B_CLEAR_STATUS(i, B_BIT_CLICK))

#define B_DOUBLE_CLICK(i)          (B_STATUS(i, B_BIT_DOUBLE_CLICK))
#define SET_B_DOUBLE_CLICK(i)      (B_SET_STATUS(i, B_BIT_DOUBLE_CLICK))
#define CLEAR_B_DOUBLE_CLICK(i)    (B_CLEAR_STATUS(i, B_BIT_DOUBLE_CLICK))

#define B_LONG_CLICK(i)          (B_STATUS(i, B_BIT_LONG_CLICK))
#define SET_B_LONG_CLICK(i)      (B_SET_STATUS(i, B_BIT_LONG_CLICK))
#define CLEAR_B_LONG_CLICK(i)    (B_CLEAR_STATUS(i, B_BIT_LONG_CLICK))

#define B_PRESS_TIME(i)            (buttons[(i)].press_time)

#define B_LAST_PRESS_TIME(i)       (buttons[(i)].last_press_time)


#define BUTTON_DOWN(button)           (!(B_CURRENT(button)))
#define BUTTON_UP(button)             (B_CURRENT(button))
#define OLD_BUTTON_DOWN(button)       (!(B_OLD(button)))
#define OLD_BUTTON_UP(button)         (B_OLD(button))
#define BUTTON_PRESSED(button)        (OLD_BUTTON_UP(button) && BUTTON_DOWN(button))
#define BUTTON_DOUBLE_CLICKED(button) (B_DOUBLE_CLICK(button))
#define BUTTON_LONG_CLICKED(button)   (B_LONG_CLICK(button))
#define BUTTON_CLICKED(button)        (B_CLICK(button))
#define BUTTON_RELEASED(button)       (OLD_BUTTON_DOWN(button) && BUTTON_UP(button))
#define BUTTON_PRESS_TIME(button)     (clock_diff(B_PRESS_TIME(button), timer2_slowclock))

extern button_t buttons[NUM_BUTTONS];
void handle_buttons(uint8_t but, button_t *buttons, uint8_t num);
void clear_buttons(void);

#endif /* HARDWARE_H__ */
