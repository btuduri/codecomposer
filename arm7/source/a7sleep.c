#pragma Ospace

#include <nds.h>
#include "def_libnds.h"
#include "a7sleep.h"

#pragma Ospace

__attribute__((noinline)) u8 PM_GetRegister(int reg)
{
	SerialWaitBusy();
 
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER |SPI_BAUD_1MHz | SPI_CONTINUOUS;
	REG_SPIDATA = reg | 0x80;
 
	SerialWaitBusy();
 
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER |SPI_BAUD_1MHz ;
	REG_SPIDATA = 0;
 
	SerialWaitBusy();

	return REG_SPIDATA & 0xff;
}
 
__attribute__((noinline)) void PM_SetRegister(int reg, int control)
{

	SerialWaitBusy();
 
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER |SPI_BAUD_1MHz | SPI_CONTINUOUS;
	REG_SPIDATA = reg;
 
	SerialWaitBusy();
 
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER |SPI_BAUD_1MHz;
	REG_SPIDATA = control;
}
 
void PM_SetControl(int control)
{
	PM_SetRegister(0, PM_GetRegister(0) | control);
}


void a7lcd_select(int control)
{
	control |= PM_GetRegister(0) & ~(PM_BACKLIGHT_BOTTOM | PM_BACKLIGHT_TOP);
	PM_SetRegister(0, control&255);
}

// 0:ON
// 1:OFF(long) and ON(short)
// 2:OFF(short) and ON(short)
// 3:OFF?

// 0:ON
// 1:OFF(long) and ON(short)
// 2:ON
// 3:OFF(short) and ON(short)
//PM_LED_CONTROL(m)
void a7led(int sw)
{
	int control = PM_LED_CONTROL(3) | BIT(7);
	int sc = sw << 4;
	control = PM_GetRegister(0) & ~control;
	PM_SetRegister(0, (control|sc)&255);
}

void a7poff(void)
{
  PM_SetControl(1<<6);//6 DS power (0: on, 1: shut down!) 
}

void a7SetSoundAmplifier(bool e)
{
	u8 control;

	// control = PM_GetRegister(0) & ~PM_SOUND_PWR;
	// if(e==true) control|=PM_SOUND_PWR;

	control = PM_GetRegister(0) & ~PM_SOUND_AMP;
	if(e==true) control|=PM_SOUND_AMP;

	PM_SetRegister(0, control&255);
}