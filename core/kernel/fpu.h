#ifndef FPU_H
#define FPU_H

void *fpu_alloc(void);
void fpu_free(void *fpuv);
int fpu_init(void);

#endif
