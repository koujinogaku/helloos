/* Interrupt controls */

#include "config.h"
#include "pic.h"
#include "cpu.h"

#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1


void pic_init(void)
/* Initialize PIC */
{
  cpu_out8(PIC0_IMR,  0xff  ); /* Don't accept all interrupt */
  cpu_out8(PIC1_IMR,  0xff  ); /* Don't accept all interrupt */

  cpu_out8(PIC0_ICW1, 0x11  ); /* Edge Trigger Mode */
  cpu_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7 accept by INT20-27 */
  cpu_out8(PIC0_ICW3, 1 << 2); /* PIC1 connects on IRQ2 */
  cpu_out8(PIC0_ICW4, 0x01  ); /* Non Buffer Mode */

  cpu_out8(PIC1_ICW1, 0x11  ); /* Edge Trigger Mode */
  cpu_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15 accept by INT28-2f */
  cpu_out8(PIC1_ICW3, 2     ); /* PIC1 connects on IRQ2 */
  cpu_out8(PIC1_ICW4, 0x01  ); /* Non Buffer Mode */

  cpu_out8(PIC0_IMR,  0xfb  ); /* 11111011 Disable all interrupt other than PIC1 */
  cpu_out8(PIC1_IMR,  0xff  ); /* 11111111 Disable all interrupt */

}

void pic_enable(int irq)
{
  if(irq < 8)
  {
    cpu_out8(PIC0_IMR, cpu_in8(PIC0_IMR) & (~ (1<<irq) )); /* Allow interrupt of PIC0 */
  }
  else
  {
    cpu_out8(PIC0_IMR,cpu_in8(PIC0_IMR) & (~(1 << 2) ));/* cascade-interrupt from PIC1 */
    cpu_out8(PIC1_IMR,cpu_in8(PIC1_IMR) & (~(1 << (irq-8)) ));/* Allow interrupt of PIC1 */
  }
}

void pic_disable(int irq)
{
  if(irq < 8)
  {
    cpu_out8(PIC0_IMR, cpu_in8(PIC0_IMR) |  (1<<irq) ); /* Disable interrupt of PIC0 */
  }
  else
  {
    cpu_out8(PIC1_IMR,cpu_in8(PIC1_IMR) | (1 << (irq-8)) );/* Disable interrupt of PIC1 */
  }
}

void pic_eoi(int irq)
{
/* Š„‚èž‚ÝŽó•tŠ®—¹‚ðPIC‚É’Ê’m */
  if(irq < 8)
  {
    cpu_out8(PIC0_OCW2, irq + 0x60);		/* notify receipt complete to PIC0 */
  }
  else
  {
    cpu_out8(PIC1_OCW2, irq - 8 + 0x60);	/* notify receipt complete to PIC1 */
    cpu_out8(PIC0_OCW2, 2 + 0x60);		/* notify receipt complete to PIC0 */
  }
}

