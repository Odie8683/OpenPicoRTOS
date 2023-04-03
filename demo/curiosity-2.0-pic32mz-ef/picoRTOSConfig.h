#ifndef PICORTOSCONFIG_H
#define PICORTOSCONFIG_H

/* CLOCKS */
#define CONFIG_SYSCLK_HZ        160000000
#define CONFIG_TICK_HZ          10000

/* TASKS */
#define CONFIG_TASK_COUNT 12

/* STACK */
#define CONFIG_DEFAULT_STACK_COUNT 256

/* IPC */
#define CONFIG_DEADLOCK_COUNT CONFIG_TICK_HZ

#endif
