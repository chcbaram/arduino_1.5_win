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
 * @file usart.c
 * @author Marti Bolivar <mbolivar@leaflabs.com>,
 *         Perry Hung <perry@leaflabs.com>
 * @brief USART control routines
 */

#include "usart.h"


#if 1

void print_byte(unsigned int c)
{
    if (c == '\n') print_byte('\r');
    while( !((*(volatile unsigned long *) 0x40013800) & 0x80) ) ;
    *(volatile unsigned long *) 0x40013804 = c;
}

char get_byte(void)
{
    while( !((*(volatile unsigned long *) 0x40013800) & 0x20) ) ;
    return (char) *(volatile unsigned long *) 0x40013804;
    
}

void SerialOutputString(const char *s);

void        printf(char *fmt, ...);
static void PrintChar(char *fmt, char c);
static void PrintDec(char *fmt, int value);
static void PrintHex(char *fmt, int value);
static void PrintString(char *fmt, char *cptr);
static int  Power(int num, int cnt);
#define SWAP8(A)                (A)
#define SWAP16(A)                ((((A)&0x00ff)<<8) | ((A)>>8))
#define SWAP32(A)                ((((A)&0x000000ff)<<24) | (((A)&0x0000ff00)<<8) | (((A)&0x00ff0000)>>8) | (((A)&0xff000000)>>24))

typedef int                        bool;
#define        true                        1
#define false                        0


// print in hex value.
// type= 8 : print in format "ff".
// type=16 : print in format "ffff".
// type=32 : print in format "ffffffff".
typedef enum {
        VAR_LONG=32,
        VAR_SHORT=16,
        VAR_CHAR=8
} VAR_TYPE;

typedef char *va_list;
#define va_start(ap, p)                (ap = (char *) (&(p)+1))
#define va_arg(ap, type)        ((type *) (ap += sizeof(type)))[-1]
#define va_end(ap)

// Write a null terminated string to the serial port.
void SerialOutputString(const char *s)
{
        while (*s != 0) 
        {
                print_byte(*s);
//            		TxDByte(*s);
                // If \n, also do \r.
                if (*s == '\n') print_byte('\r');
//                if (*s == '\n') TxDByte('\r');
                s++;        
        }
} // SerialOutputString.

// ���ڿ� s1, s2�� ���� len�� ���� �̳����� ��.
// return : 0 : equil                ret : s1 > s2                -ret : s1 < s2
int StrNCmp(char *s1, char *s2, int len){
        int i;


        for(i = 0; i < len; i++){
                if(s1[i] != s2[i])        return ((int)s1[i]) - ((int)s2[i]);
                if(s1[i] == 0)                return 0;
        }
        return 0;
} // StrNCmp.

// ���ڿ� s1, s2�� ��.
// return : 0 : equil                ret : s1 > s2                -ret : s1 < s2
int StrCmp(char *s1, char *s2){
        for (; *s1 && *s2; s1++, s2++){
                if (*s1 != *s2) return ((int)(*s1) - (int)(*s2));
        }
        if (*s1 || *s2) return ((int)(*s1) - (int)(*s2));
        return 0;
}        // StrCmp.

// ���� : 10���� ���ڿ� s���� ������ ����� retval�� ����Ű�� ��ġ�� ���.
// �Ű� : s      : ��ȯ�� ���ڿ��� �ּ�.
//        retval : ��ȯ�� ���� ��ϵ� �ּ�.
// ��ȯ : return : 1 : success                0 : failure.
// ���� :
int DecToLong(char *s, long *retval){
        long remainder;
        if (!s || !s[0]) return false;


        for (*retval=0; *s; s++){
                if (*s < '0' || *s > '9') return false;
                remainder = *s - '0';
                *retval = *retval * 10 + remainder;
        }


        return true;
}        // DecToLong.

// ���� : printf() �� �Ϻθ� �����ϰ� ����.
// �Ű� : fmt : printf()�� �����ϳ� "%s", "%c", "%d", "%x" ��� ����.
//              %d, %x�� ��쿡�� "%08x", "%8x"�� ���� ��Ÿ�� ���̿� �� ������ 0���� ä���� ���� ����.
// ��ȯ : ����.
// ���� : ����.
void printf(char *fmt, ...)
{
        int                i;
        va_list args;
        char        *s=fmt;
        char        format[10];        // fmt�� ���ڰ� "%08lx"���, "08l"�� �ӽ÷� ���.
        
        va_start(args, fmt);
        while (*s){
                if (*s=='%'){
                        s++;
                        // s���� "%08lx"������ ������ format�� ���. ���߿� ����Լ��� �Ѱ���.
                        format[0] = '%';
                        for (i=1; i<10;){
                                if (*s=='c' || *s=='d' || *s=='x' || *s=='s' || *s=='%'){
                                        format[i++] = *s;
                                        format[i] = '\0';
                                        break;
                                }
                                else {
                                        format[i++] = *s++;
                                }
                        }
                        // "%s", "%c", "%d", "%x"�� ã�� ����� �Լ� ȣ��.
                        switch (*s++){
                                case 'c' :
                                        PrintChar(format, va_arg(args, int));
                                        break;
                                case 'd' :
                                        PrintDec(format, va_arg(args, int));
                                        break;
                                case 'x' :
                                        PrintHex(format, va_arg(args, int));
                                        break;
                                case 's' :
                                        PrintString(format, va_arg(args, char *));
                                        break;
                                case '%' :
                                        PrintChar("%c", '%');
                                        break;
                        }
                }
                else {
                        PrintChar("%c", *s);
                        s++;
                }
        }
        va_end(args);
        return;
}

void PrintChar(char *fmt, char c)
{
        print_byte(c);
//        TxDByte(c);
        return;
}

void PrintDec(char *fmt, int l)
{
        int        i, j;
        char        c, *s=fmt, tol[10];
        bool        flag0=false, flagl=false;        // "%08lx"���� '0', 'l'�� ���� ����.
        long        flagcnt=0;                                        // "%08lx"���� "8"�� ã�Ƽ� long������.
        bool        leading_zero=true;                        // long���� data�� ����ϱ� ���� ����.
        long        divisor, result, remainder;


        // fmt�� "%08lx"���� '0', '8', 'l'�� �ؼ�.
        for (i=0; (c=s[i]) != 0; i++){
                if (c=='d') break;
                else if (c>='1' && c<='9'){
                        for (j=0; s[i]>='0' && s[i]<='9'; j++){
                                tol[j] = s[i++];
                        }
                        tol[j] = '\0';
                        i--;
                        DecToLong(tol, &flagcnt);
                }
                else if (c=='0') flag0=true;
                else if (c=='l') flagl=true;
                else continue;
        }


        // ���� flag�� ���� ���.
        if (flagcnt){
                if (flagcnt>9) flagcnt=9;
                remainder = l%(Power(10, flagcnt));        // flagcnt���� ���ڸ��� ���� �ɷ���. 199�� flagcnt==2�̸�, 99��.


                for (divisor=Power(10, flagcnt-1); divisor>0; divisor/=10){
                        result = remainder/divisor;
                        remainder %= divisor;


                        if (result!=0 || divisor==1) leading_zero = false;


                        if (leading_zero==true){
                                if (flag0)        print_byte('0');
//                                if (flag0)        TxDByte('0');
                                else                print_byte(' ');
//                                else                TxDByte(' ');
                        }
                        else print_byte((char)(result)+'0');
//                        else TxDByte((char)(result)+'0');
                }
        } else {
                remainder = l;


                for (divisor=1000000000; divisor>0; divisor/=10){
                        result = remainder/divisor;
                        remainder %= divisor;


                        if (result!=0 || divisor==1) leading_zero = false;
                        if (leading_zero==false) print_byte((char)(result)+'0');
//                        if (leading_zero==false) TxDByte((char)(result)+'0');
                }
        }
        return;
}

void PrintHex(char *fmt, int l){
        int                i, j;
        char        c, *s=fmt, tol[10];
        bool        flag0=false, flagl=false;        // flags.
        long        flagcnt=0;
        bool        leading_zero=true;
        char        uHex, lHex;
        int                cnt;                                                // "%5x"�� ��� 5���� ����ϵ��� ����� ����.


        // fmt�� "%08lx"���� '0', '8', 'l'�� �ؼ�.
        for (i=0; (c=s[i]) != 0; i++){
                if (c=='x') break;
                else if (c>='1' && c<='9'){
                        for (j=0; s[i]>='0' && s[i]<='9'; j++){
                                tol[j] = s[i++];
                        }
                        tol[j] = '\0';
                        i--;
                        DecToLong(tol, &flagcnt);
                }
                else if (c=='0') flag0=true;
                else if (c=='l') flagl=true;
                else continue;
        }


        s = (char *)(&l);
        l = SWAP32(l);                // little, big endian�� ����.(big�� ����ϱ� ���� ������ �ٲ�)
        
        // ���� flag�� ���� ���.
        if (flagcnt){
                if (flagcnt&0x01){        // flagcnt�� Ȧ�� �϶�, upper�� ����, lower�� ���.
                        c = s[(8-(flagcnt+1))/2]; // Ȧ�� �϶� �� ��ġ�� �����ϴ� ���� ���� ���� �ɴϴ�.
                        
                        // lower 4 bits�� �����ͼ� ascii code��.
                        lHex = ((c>>0)&0x0f);
                        if (lHex!=0) leading_zero=false;
                        if (lHex<10) lHex+='0';
                        else         lHex+='A'-10;


                        // lower 4 bits ���.
                        if (leading_zero){
                                if (flag0) print_byte('0');
//                                if (flag0) TxDByte('0');
                                else       print_byte(' ');
//                                else       TxDByte(' ');
                        }
                        else print_byte(lHex);
//                        else TxDByte(lHex);
                        
                        flagcnt--;
                }


                // byte������ data�� Hex�� ���.
                for (cnt=0, i=(8-flagcnt)/2; i<4; i++){
                        c = s[i];
                                
                        // get upper 4 bits and lower 4 bits.
                        uHex = ((c>>4)&0x0f);
                        lHex = ((c>>0)&0x0f);


                        // upper 4 bits and lower 4 bits to '0'~'9', 'A'~'F'.
                        // upper 4 bits�� ascii code��.
                        if (uHex!=0) leading_zero = false;
                        if (uHex<10) uHex+='0';
                        else         uHex+='A'-10;


                        // upper 4 bits ���.
                        if (leading_zero){
                                if (flag0) print_byte('0');
//                                if (flag0) TxDByte('0');
                                else       print_byte(' ');
//                                else       TxDByte(' ');
                        }
                        else print_byte(uHex);
//                        else TxDByte(uHex);
                        
                        // lower 4 bits�� ascii code��.
                        if (lHex!=0) leading_zero = false;
                        if (lHex<10) lHex+='0';
                        else         lHex+='A'-10;


                        // lower 4 bits ���.
                        if (leading_zero){
                                if (flag0) print_byte('0');
//                                if (flag0) TxDByte('0');
                                else       print_byte(' ');
//                                else       TxDByte(' ');
                        }
                        else print_byte(lHex);
//                        else TxDByte(lHex);
                }
        }
        else {
                for (i=0; i<4; i++){
                        c = s[i];
        
                        // get upper 4 bits and lower 4 bits.
                        uHex = ((c>>4)&0x0f);
                        lHex = ((c>>0)&0x0f);


                        // upper 4 bits and lower 4 bits to '0'~'9', 'A'~'F'.
                        if (uHex!=0) leading_zero = false;
                        if (uHex<10) uHex+='0';
                        else         uHex+='A'-10;
                        if (!leading_zero) print_byte(uHex);
//                        if (!leading_zero) TxDByte(uHex);
                        
                        if (lHex!=0 || i==3) leading_zero = false;
                        if (lHex<10) lHex+='0';
                        else         lHex+='A'-10;
                        if (!leading_zero) print_byte(lHex);
//                        if (!leading_zero) TxDByte(lHex);
                }
        }
        return;
}

void PrintString(char *fmt, char *s){
        if (!fmt || !s) return;
        while (*s) print_byte(*s++);
//        while (*s) TxDByte(*s++);
        return;
}

int Power(int num, int cnt){
        long retval=num;
        cnt--;


        while (cnt--){
                retval *= num;
        }
        return retval;
} 




//-----------------------------------------------------------------------------
void HexDump(char *addr, int len)
{
//#if 0
    char *s=addr, *endPtr=(char *)((long)addr+len);
    int  i, remainder=len%16;

    printf("\n");
    printf("Offset      Hex Value                                        Ascii value\n");

    // print out 16 byte blocks.
    while (s+16<=endPtr)
    {
        // offset ���.
        printf("0x%08lx  ", (long)(s-addr));
        // 16 bytes ������ ���� ���.
        for (i=0; i<16; i++)
        {
            printf("%02x ", s[i]);
        }
        printf(" ");
        for (i=0; i<16; i++)
        {               
            if   (s[i]>=32 && s[i]<=125) printf("%c", s[i]);
            else                         printf(".");
        }
        s += 16;
        printf("\n");
    }

    // Print out remainder.
    if (remainder)
    {
        // offset ���.
        printf("0x%08lx  ", (long)(s-addr));
        // 16 bytes ������ ����ϰ� ���� �� ���.
        for (i=0; i<remainder; i++)
        {
            printf("%02x ", s[i]);
        }
        for (i=0; i<(16-remainder); i++)
        {
            printf("   ");
        }
        printf(" ");
        for (i=0; i<remainder; i++)
        {
            if   (s[i]>=32 && s[i]<=125) printf("%c", s[i]);
            else                         printf(".");
        }
        for (i=0; i<(16-remainder); i++)
        {
            printf(" ");
        }
        printf("\n");
    }
    return;
//#endif
}




#endif






voidFuncPtrUart userUsartInterrupt1 = NULL;
voidFuncPtrUart userUsartInterrupt2 = NULL;
voidFuncPtrUart userUsartInterrupt3 = NULL;
#ifdef STM32_HIGH_DENSITY
voidFuncPtrUart userUsartInterrupt4 = NULL;
voidFuncPtrUart userUsartInterrupt5 = NULL;
#endif



/*
 * Devices
 */


static ring_buffer usart1_rb;
static usart_dev usart1 = {
    .regs     = USART1_BASE,
    .rb       = &usart1_rb,
    .max_baud = 4500000UL,
    .clk_id   = RCC_USART1,
    .irq_num  = NVIC_USART1
};
/** USART1 device */
usart_dev *USART1 = &usart1;

static ring_buffer usart2_rb;
static usart_dev usart2 = {
    .regs     = USART2_BASE,
    .rb       = &usart2_rb,
    .max_baud = 2250000UL,
    .clk_id   = RCC_USART2,
    .irq_num  = NVIC_USART2
};
/** USART2 device */
usart_dev *USART2 = &usart2;

static ring_buffer usart3_rb;
static usart_dev usart3 = {
    .regs     = USART3_BASE,
    .rb       = &usart3_rb,
    .max_baud = 2250000UL,
    .clk_id   = RCC_USART3,
    .irq_num  = NVIC_USART3
};
/** USART3 device */
usart_dev *USART3 = &usart3;

#ifdef STM32_HIGH_DENSITY
static ring_buffer uart4_rb;
static usart_dev uart4 = {
    .regs     = UART4_BASE,
    .rb       = &uart4_rb,
    .max_baud = 2250000UL,
    .clk_id   = RCC_UART4,
    .irq_num  = NVIC_UART4
};
/** UART4 device */
usart_dev *UART4 = &uart4;

static ring_buffer uart5_rb;
static usart_dev uart5 = {
    .regs     = UART5_BASE,
    .rb       = &uart5_rb,
    .max_baud = 2250000UL,
    .clk_id   = RCC_UART5,
    .irq_num  = NVIC_UART5
};
/** UART5 device */
usart_dev *UART5 = &uart5;
#endif

/**
 * @brief Initialize a serial port.
 * @param dev         Serial port to be initialized
 */
void usart_init(usart_dev *dev) {
    rb_init(dev->rb, USART_RX_BUF_SIZE, dev->rx_buf);
    rcc_clk_enable(dev->clk_id);
    nvic_irq_enable(dev->irq_num);
}

/**
 * @brief Configure a serial port's baud rate.
 *
 * @param dev         Serial port to be configured
 * @param clock_speed Clock speed, in megahertz.
 * @param baud        Baud rate for transmit/receive.
 */
void usart_set_baud_rate(usart_dev *dev, uint32 clock_speed, uint32 baud) {
    uint32 integer_part;
    uint32 fractional_part;
    uint32 tmp;

    /* See ST RM0008 for the details on configuring the baud rate register */
    integer_part = (25 * clock_speed) / (4 * baud);
    tmp = (integer_part / 100) << 4;
    fractional_part = integer_part - (100 * (tmp >> 4));
    tmp |= (((fractional_part * 16) + 50) / 100) & ((uint8)0x0F);

    //TxDStringC("tmp clock = ");TxDHex16C(tmp);TxDStringC("\r\n");
    dev->regs->BRR = (uint16)tmp;
}

/**
 * @brief Enable a serial port.
 *
 * USART is enabled in single buffer transmission mode, multibuffer
 * receiver mode, 8n1.
 *
 * Serial port must have a baud rate configured to work properly.
 *
 * @param dev Serial port to enable.
 * @see usart_set_baud_rate()
 */
void usart_enable(usart_dev *dev) {
    usart_reg_map *regs = dev->regs;
    regs->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    regs->CR1 |= USART_CR1_UE;
}

/**
 * @brief Turn off a serial port.
 * @param dev Serial port to be disabled
 */
void usart_disable(usart_dev *dev) {
    /* FIXME this misbehaves if you try to use PWM on TX afterwards */
    usart_reg_map *regs = dev->regs;

    /* TC bit must be high before disabling the USART */
    while((regs->CR1 & USART_CR1_UE) && !(regs->SR & USART_SR_TC))
        ;

    /* Disable UE */
    regs->CR1 &= ~USART_CR1_UE;

    /* Clean up buffer */
    usart_reset_rx(dev);
}

/**
 *  @brief Call a function on each USART.
 *  @param fn Function to call.
 */
void usart_foreach(void (*fn)(usart_dev*)) {
    fn(USART1);
    fn(USART2);
    fn(USART3);
#ifdef STM32_HIGH_DENSITY
    fn(UART4);
    fn(UART5);
#endif
}

/**
 * @brief Nonblocking USART transmit
 * @param dev Serial port to transmit over
 * @param buf Buffer to transmit
 * @param len Maximum number of bytes to transmit
 * @return Number of bytes transmitted
 */
uint32 usart_tx(usart_dev *dev, const uint8 *buf, uint32 len) {

	//TxDHex8C(buf[0]);
    usart_reg_map *regs = dev->regs;
    uint32 txed = 0;
    while ((regs->SR & USART_SR_TXE) && (txed < len)) {
        regs->DR = buf[txed++];
    }
    return txed;
}

/**
 * @brief Transmit an unsigned integer to the specified serial port in
 *        decimal format.
 *
 * This function blocks until the integer's digits have been
 * completely transmitted.
 *
 * @param dev Serial port to send on
 * @param val Number to print
 */
void usart_putudec(usart_dev *dev, uint32 val) {
    char digits[12];
    int i = 0;

    do {
        digits[i++] = val % 10 + '0';
        val /= 10;
    } while (val > 0);

    while (--i >= 0) {
        usart_putc(dev, digits[i]);
    }
}



/**
 * @brief Attach a USART interrupt.
 * @param dev USART device
 * @param interrupt Interrupt number to attach to USART DeviceX
 * @param handler Handler to attach to the given interrupt.
 *
 */
void usart_attach_interrupt(usart_dev *dev,
                            voidFuncPtrUart handler) {

	if ( dev == USART1){
		userUsartInterrupt1 = handler;
	}
	else if(dev == USART2){
		userUsartInterrupt2 = handler;
	}
	else if(dev == USART3){
		userUsartInterrupt3 = handler;
	}
#ifdef STM32_HIGH_DENSITY
	else if(dev == USART4){
		userUsartInterrupt4 = handler;
	}
	else if(dev == USART5){
		userUsartInterrupt5 = handler;
	}
#endif
}
/*
 * @brief detach a USART interrupt.
 * @param dev USART device
 * */
void usart_detach_interrupt(usart_dev *dev){
	if ( dev == USART1){
		userUsartInterrupt1 = NULL;
	}
	else if(dev == USART2){
		userUsartInterrupt2 = NULL;
	}
	else if(dev == USART3){
		userUsartInterrupt3 = NULL;
	}
#ifdef STM32_HIGH_DENSITY
	else if(dev == USART4){
		userUsartInterrupt4 = NULL;
	}
	else if(dev == USART5){
		userUsartInterrupt5 = NULL;
	}
#endif

}



/*
 * Interrupt handlers.
 */

static inline void usart_irq(usart_dev *dev) {
#ifdef USART_SAFE_INSERT
    /* If the buffer is full and the user defines USART_SAFE_INSERT,
     * ignore new bytes. */
    rb_safe_insert(dev->rb, (uint8)dev->regs->DR);
#else
    /* By default, push bytes around in the ring buffer. */

    rb_push_insert(dev->rb, (uint8)dev->regs->DR);
#endif
}

void __irq_usart1(void) {

	//TxDByteC();
	//TxDStringC("usart1 irq\r\n");
	if(userUsartInterrupt1 != NULL){
		userUsartInterrupt1((byte)USART1->regs->DR);
	}
    usart_irq(USART1);

}

void __irq_usart2(void) {

	if(userUsartInterrupt2 != NULL){
		userUsartInterrupt2((byte)USART2->regs->DR);
	}


    usart_irq(USART2);
}

void __irq_usart3(void) {
	if(userUsartInterrupt3 != NULL){
		userUsartInterrupt3((byte)USART3->regs->DR);
	}
    usart_irq(USART3);
}

#ifdef STM32_HIGH_DENSITY
void __irq_uart4(void) {
	if(userUsartInterrupt4 != NULL){
		userUsartInterrupt4((byte)USART4->regs->DR);
	}
    usart_irq(UART4);
}

void __irq_uart5(void) {
	if(userUsartInterrupt5 != NULL){
		userUsartInterrupt5((byte)USART5->regs->DR);
	}
    usart_irq(UART5);
}
#endif
