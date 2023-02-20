#include "picoRTOS.h"
#include "curiosity-2.0-pic32mz-ef.h"

#include "picoRTOS_mutex.h"
#include "picoRTOS_cond.h"

static struct picoRTOS_mutex mutex = PICORTOS_MUTEX_INITIALIZER;
static struct picoRTOS_cond cond = PICORTOS_COND_INITIALIZER;

static void led1_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL);

    struct gpio *LED = (struct gpio*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    for (;;) {
        /* Pattern 1 */
        picoRTOS_mutex_lock(&mutex);

        gpio_write(LED, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(30));
        gpio_write(LED, true);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(30));

        gpio_write(LED, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(60));
        gpio_write(LED, true);

        picoRTOS_cond_signal(&cond);
        picoRTOS_mutex_unlock(&mutex);

        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_SEC(1));
    }
}

static void led2_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL);

    struct gpio *LED = (struct gpio*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    for (;;) {
        /* Pattern 2 */
        gpio_toggle(LED);
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_MSEC(500));
    }
}

static void led3_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL);

    struct gpio *LED = (struct gpio*)priv;

    for (;;) {
        /* Pattern 1 */
        picoRTOS_mutex_lock(&mutex);
        picoRTOS_cond_wait(&cond, &mutex);

        gpio_write(LED, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(30));
        gpio_write(LED, true);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(30));

        gpio_write(LED, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(60));
        gpio_write(LED, true);

        picoRTOS_mutex_unlock(&mutex);
    }
}

/*
 * Fades in and out the RGB LED.
 * The LED is inverted, the best option would be to invert the PWM, but it doesn't seem possible
 * on that board
 */
static void led4_main(void *priv)
{
#define LED4_PWM_COUNT 3

    picoRTOS_assert_fatal(priv != NULL);

    picoRTOS_tick_t ref = picoRTOS_get_tick();
    struct rgb_led *LED4 = (struct rgb_led*)priv;

    bool fade_in = true;
    unsigned long duty_cycle_pcent = 0;

    pwm_start(&LED4->R);
    pwm_start(&LED4->G);
    pwm_start(&LED4->B);

    for (;;) {
        (void)pwm_set_duty_cycle(&LED4->R, PWM_DUTY_CYCLE_PCENT(duty_cycle_pcent));
        (void)pwm_set_duty_cycle(&LED4->G, PWM_DUTY_CYCLE_PCENT(duty_cycle_pcent));
        (void)pwm_set_duty_cycle(&LED4->B, PWM_DUTY_CYCLE_PCENT(duty_cycle_pcent));

        if (fade_in) {
            if (++duty_cycle_pcent == 100ul)
                fade_in = false;

        }else if (--duty_cycle_pcent == 0)
            fade_in = true;

        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_MSEC(5)); /* 200Hz */
    }
}

/*
 * console_main is a thread that simply echoes characters
 */
static void console_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL);

    struct uart *UART = (struct uart*)priv;

    for (;;) {

        char c = (char)0x0;

        /* just echo */
        if (uart_read(UART, &c, sizeof(c)) == -EAGAIN) {
            picoRTOS_schedule();
            continue;
        }

        (void)uart_write(UART, &c, sizeof(c));
    }
}

/*
 * spi_main is a thread that sends data to the SPI and expects to get it back.
 * No loopback is present on the pic32mx, so you must setup a wire between pins MISO & MOSI
 * on mikrobus
 */
static void spi_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL);

    size_t xfered = 0;
    struct spi *SPI = (struct spi*)priv;
    char rx[6] = { (char)0, (char)0, (char)0, (char)0, (char)0, (char)0 };
    char tx[6] = { (char)0xa5, (char)0x55, (char)0x5a, (char)0x55, (char)0x4d, (char)0x4e };

    for (;;) {
        int res;

        if ((res = spi_xfer(SPI, &rx[xfered], &tx[xfered], sizeof(tx) - xfered)) == -EAGAIN) {
            picoRTOS_schedule();
            continue;
        }

        picoRTOS_assert_fatal(res > 0);

        /* ack xfer */
        xfered += (size_t)res;

        if (xfered == sizeof(tx)) {
            picoRTOS_assert_fatal(rx[0] == (char)0xa5);
            picoRTOS_assert_fatal(rx[4] == (char)0x4d);
            /* start again */
            xfered = 0;
        }
    }
}

/*
 * Watchdog refreshing task. Should be run last, just in case
 */
static void wd_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL);

    struct wd *WDT = (struct wd*)priv;

    for (;;) {
        wd_refresh(WDT);
        picoRTOS_schedule();
    }
}

/*
 * pwm_main put a ramp on a PWM
 */
static void pwm_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL);

    pwm_duty_cycle_t duty_cycle = 0;
    struct pwm *PWM = (struct pwm*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    pwm_start(PWM);

    for (;;) {
        (void)pwm_set_duty_cycle(PWM, duty_cycle);

        duty_cycle += PWM_DUTY_CYCLE_PCENT(1);
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_MSEC(10)); /* 100Hz */
    }
}

int main(void)
{
    static struct curiosity_20_pic32mz_ef board;

    /* Init the system */
    picoRTOS_init();
    (void)curiosity_20_pic32mz_ef_init(&board);

    struct picoRTOS_task task;
    static picoRTOS_stack_t stack0[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack1[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack2[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack3[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack4[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack5[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack6[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack7[CONFIG_DEFAULT_STACK_COUNT];

    picoRTOS_task_init(&task, led1_main, &board.LED1, stack0, PICORTOS_STACK_COUNT(stack0));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    picoRTOS_task_init(&task, led2_main, &board.LED2, stack1, PICORTOS_STACK_COUNT(stack1));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    picoRTOS_task_init(&task, led3_main, &board.LED3, stack2, PICORTOS_STACK_COUNT(stack2));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    picoRTOS_task_init(&task, led4_main, &board.LED4, stack3, PICORTOS_STACK_COUNT(stack3));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    picoRTOS_task_init(&task, console_main, &board.UART6, stack4, PICORTOS_STACK_COUNT(stack4));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    picoRTOS_task_init(&task, spi_main, &board.SPI1, stack5, PICORTOS_STACK_COUNT(stack5));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    picoRTOS_task_init(&task, pwm_main, &board.PWM1, stack6, PICORTOS_STACK_COUNT(stack6));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    /* lastly, watchdog */
    picoRTOS_task_init(&task, wd_main, &board.WDT, stack7, PICORTOS_STACK_COUNT(stack7));
    picoRTOS_add_task(&task, picoRTOS_get_last_available_priority());

    /* start the scheduler */
    picoRTOS_start();

    /* we're not supposed to end here */
    picoRTOS_assert_fatal(false);
    return -1;
}
