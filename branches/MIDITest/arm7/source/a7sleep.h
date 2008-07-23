
#include <NDS.h>

extern u8 PM_GetRegister(int reg);
extern void PM_SetRegister(int reg, int control);

//void a7sleep_dummy(void);
//void a7sleep(void);

void a7lcd_select(int control);
void a7led(int sw);
void a7poff(void);
void a7SetSoundAmplifier(bool e);