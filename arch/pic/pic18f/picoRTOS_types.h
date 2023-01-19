#ifndef PICORTOS_TYPES_H
#define PICORTOS_TYPES_H

/* optimize for speed */
typedef unsigned char picoRTOS_stack_t;
typedef unsigned int picoRTOS_tick_t;
typedef unsigned int picoRTOS_priority_t;
typedef unsigned char picoRTOS_atomic_t;
typedef unsigned int picoRTOS_irq_t;
typedef unsigned int picoRTOS_cycles_t;

#define ARCH_INTIAL_STACK_COUNT 36
#define ARCH_MIN_STACK_COUNT (ARCH_INTIAL_STACK_COUNT + 6)

/* splint cannot check inline assembly */
#ifdef S_SPLINT_S
# define ASM(x) {}
#else
# define ASM(x) __asm__ volatile (x)
# define arch_break() ({ ASM("nop"); })
#endif

#endif