#ifndef PIC_H
#define PIC_H

/* pic.c */
void pic_init(void);
void pic_enable(int irq);
void pic_disable(int irq);
void pic_eoi(int irq);

#define PIC_IRQ_TIMER	0x00
#define PIC_IRQ_KEYBRD	0x01
#define PIC_IRQ_FDC	0x06
#define PIC_IRQ_RTC	0x08
#define PIC_IRQ_MOUSE	0x0c
#define PIC_IRQ_FPU	0x0d
#define PIC_IRQ_IDE0	0x0e
#define PIC_IRQ_IDE1	0x0f

#endif
