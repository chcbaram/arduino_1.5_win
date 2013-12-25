/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @brief Generic board initialization routines.
 *
 * By default, we bring up all Maple boards to 72MHz, clocked off the
 * PLL, driven by the 8MHz external crystal. AHB and APB2 are clocked
 * at 72MHz.  APB1 is clocked at 36MHz.
 */

#include "boards.h"

#include "flash.h"
#include "rcc.h"
#include "nvic.h"
#include "systick.h"
#include "gpio.h"
#include "adc.h"
#include "timer.h"
#include "usb.h"

//for debug
#include "usart.h"
#include "wirish_time.h"

static void setupFlash(void);
static void setupClocks(void);
static void setupNVIC(void);
static void setupADC(void);
static void setupTimers(void);


 void TxDByte(uint8 buf){

	   while (!usart_tx(USART2, &buf, 1))
	        ;
}

void TxDString(char *str)
{
	int i;
	for(i=0; str[i] ; i++)
	{
		TxDByte(str[i]);
	}
}

void init(void) {

/////// 추가 시작
*(volatile unsigned long *) 0x40021018 |= 0x1 << 14;         // uart/ IOPD EN/ IOPA EN / AFIO EN

	/* Configure the GPIO ports */
*(volatile unsigned long *) 0x40010804 &= ~(0xFF << 4);                    //Port A(8-15)
*(volatile unsigned long *) 0x40010804 |= (0x4B << 4);                    //Port A(8-15)
*(volatile unsigned long *) 0x40013810 = 0x0;         // 1 stop bit
*(volatile unsigned long *) 0x4001380C = 0x200C;   //8bit no parity

*(volatile unsigned long *) 0x40013814 = 0x0;
*(volatile unsigned long *) 0x40013808 = 39 << 4 | 1;   //USART1->BBR register

*(volatile unsigned long *) 0x4001380C |= 0x2000;

/////// 추가 끝

//    printf("setupFlash\n");
    setupFlash();

//    printf("setupClocks\n");
    setupClocks();
//    printf("setupNVIC\n");
    setupNVIC();
//    printf("systick_init\n");
    systick_init(SYSTICK_RELOAD_VAL);
//    printf("gpio_init_all\n");
    gpio_init_all();
*(volatile unsigned long *) 0x40010804 &= ~(0xFF << 4);                    //Port A(8-15)
*(volatile unsigned long *) 0x40010804 |= (0x4B << 4);                    //Port A(8-15)

//    printf("afio_init\n");
    afio_init();
//    printf("setupADC\n");
    setupADC();
//    printf("setupTimers\n");
    setupTimers();
//    printf("setupUSB\n");
    setupUSB();
//    printf("boardInit\n");
    boardInit();

    //for debug
//    printf("gpio_set_mode\n");
    gpio_set_mode(GPIOA, 2, GPIO_AF_OUTPUT_PP);
//    printf("gpio_set_mode\n");
 	gpio_set_mode(GPIOA, 3, GPIO_INPUT_FLOATING);
//    printf("gpio_set_mode\n");


 	usart_init(USART2);
//    printf("usart_set_baud_rate\n");
 	usart_set_baud_rate(USART2, STM32_PCLK1, 57600);
//    printf("usart_enable\n");
 	usart_enable(USART2);
}

/* You could farm this out to the files in boards/ if e.g. it takes
 * too long to test on Maple Native (all those FSMC pins...). */
bool boardUsesPin(uint8 pin) {
    for (int i = 0; i < BOARD_NR_USED_PINS; i++) {
        if (pin == boardUsedPins[i]) {
            return true;
        }
    }
    return false;
}

static void setupFlash(void) {
    flash_enable_prefetch();
    flash_set_latency(FLASH_WAIT_STATE_2);
}

/*
 * Clock setup.  Note that some of this only takes effect if we're
 * running bare metal and the bootloader hasn't done it for us
 * already.
 *
 * If you change this function, you MUST change the file-level Doxygen
 * comment above.
 */
static void setupClocks() {
    rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSE, RCC_PLLMUL_9);
    rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB1_HCLK_DIV_2);
    rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);
}

static void setupNVIC() {
#ifdef VECT_TAB_FLASH  //this define is coming from compile -D option
	nvic_init(NVIC_VectTab_FLASH, 0x3000);//nvic_init(USER_ADDR_ROM, 0);
#elif defined VECT_TAB_RAM
    nvic_init(USER_ADDR_RAM, 0);
#elif defined VECT_TAB_BASE
    nvic_init((uint32)0x08000000, 0);
#else
#error "You must select a base address for the vector table."
#endif
}

static void adcDefaultConfig(const adc_dev* dev);

static void setupADC() {
    rcc_set_prescaler(RCC_PRESCALER_ADC, RCC_ADCPRE_PCLK_DIV_6);
    adc_foreach(adcDefaultConfig);
}

static void timerDefaultConfig(timer_dev*);

static void setupTimers() {
    timer_foreach(timerDefaultConfig);
}

static void adcDefaultConfig(const adc_dev *dev) {
    adc_init(dev);

    adc_set_extsel(dev, ADC_SWSTART);
    adc_set_exttrig(dev, true);

    adc_enable(dev);
    adc_calibrate(dev);
    adc_set_sample_rate(dev, ADC_SMPR_55_5);
}

static void timerDefaultConfig(timer_dev *dev) {
    timer_adv_reg_map *regs = (dev->regs).adv;
    const uint16 full_overflow = 0xFFFF;
    const uint16 half_duty = 0x8FFF;

    timer_init(dev);
    timer_pause(dev);

    regs->CR1 = TIMER_CR1_ARPE;
    regs->PSC = 1;
    regs->SR = 0;
    regs->DIER = 0;
    regs->EGR = TIMER_EGR_UG;

    switch (dev->type) {
    case TIMER_ADVANCED:
        regs->BDTR = TIMER_BDTR_MOE | TIMER_BDTR_LOCK_OFF;
        // fall-through
    case TIMER_GENERAL:
        timer_set_reload(dev, full_overflow);

        for (int channel = 1; channel <= 4; channel++) {
            timer_set_compare(dev, channel, half_duty);
            timer_oc_set_mode(dev, channel, TIMER_OC_MODE_PWM_1, TIMER_OC_PE);
        }
        // fall-through
    case TIMER_BASIC:
        break;
    }

    timer_resume(dev);
}
