#include "curiosity-2.0-pic32mz-ef.h"
#include "picoRTOS_device.h"

#include "clock-pic32mx.h"
#include "mux-pic32mx.h"

static void clock_init(void)
{
    struct clock_settings CLOCK_settings = {
        12000000ul,     /* Posc */
        CLOCK_PIC32MX_PLLICLK_FRC,
        200000000ul,    /* PLL output req freq */
        CLOCK_PIC32MX_OSC_SPLL
    };

    (void)clock_pic32mx_init(&CLOCK_settings);

    /* UART & SPI Baud Rate Generator */
    (void)clock_pic32mx_pbclk_enable(CLOCK_PIC32MX_PBCLK2, 2ul);
    /* OC/PWM */
    (void)clock_pic32mx_pbclk_enable(CLOCK_PIC32MX_PBCLK3, 2ul);
    /* PORT control & timer */
    (void)clock_pic32mx_pbclk_enable(CLOCK_PIC32MX_PBCLK4, 2ul);
}

static void mux_init(void)
{
    struct mux_pic32mx PORTB;
    struct mux_pic32mx PORTC;
    struct mux_pic32mx PORTD;
    struct mux_pic32mx PORTF;
    struct mux_pic32mx PORTJ;
    struct mux_pic32mx PORTK;

    (void)mux_pic32mx_init(&PORTB, (struct MUX_PIC32MX*)ADDR_PORTB);
    (void)mux_pic32mx_init(&PORTC, (struct MUX_PIC32MX*)ADDR_PORTC);
    (void)mux_pic32mx_init(&PORTD, (struct MUX_PIC32MX*)ADDR_PORTD);
    (void)mux_pic32mx_init(&PORTF, (struct MUX_PIC32MX*)ADDR_PORTF);
    (void)mux_pic32mx_init(&PORTJ, (struct MUX_PIC32MX*)ADDR_PORTJ);
    (void)mux_pic32mx_init(&PORTK, (struct MUX_PIC32MX*)ADDR_PORTK);

    (void)mux_pic32mx_output(&PORTJ, (size_t)7);    /* LED1 */
    (void)mux_pic32mx_output(&PORTK, (size_t)7);    /* LED2 */
    (void)mux_pic32mx_output(&PORTJ, (size_t)3);    /* LED3 */

    /* UART1 */
    (void)mux_pic32mx_input(&PORTC, (size_t)1);         /* RX */
    (void)mux_pic32mx_pull_up(&PORTC, (size_t)1);
    (void)mux_pic32mx_peripheral_pin_select_input(MUX_PIC32MX_PPSI_U1RXR, (size_t)0xa);
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPC4R, (size_t)0x1);

    /* UART6 */
    (void)mux_pic32mx_input(&PORTF, (size_t)13);         /* RX */
    //(void)mux_pic32mx_pull_up(&PORTC, (size_t)13);
    (void)mux_pic32mx_peripheral_pin_select_input(MUX_PIC32MX_PPSI_U6RXR, (size_t)0x9);
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPF2R, (size_t)0x4);

    /* SPI1 */
    (void)mux_pic32mx_input(&PORTD, (size_t)14);
    (void)mux_pic32mx_peripheral_pin_select_input(MUX_PIC32MX_PPSI_SDI1R, (size_t)0xb);     /* MISO */
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPD7R, (size_t)0x5);    /* MOSI */
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPD1R, (size_t)0x0);    /* SCK */
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPD9R, (size_t)0x5);    /* SS */

    /* PWM */
    (void)mux_pic32mx_output(&PORTB, (size_t)6);
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPB6R, (size_t)0xc);    /* OC1 */
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPB7R, (size_t)0xb);    /* LED_R */
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPB8R, (size_t)0xc);    /* LED_G */
    (void)mux_pic32mx_peripheral_pin_select_output(MUX_PIC32MX_PPSO_RPB9R, (size_t)0xb);    /* LED_B */
}

static int gpio_init(/*@partial@*/ struct curiosity_20_pic32mz_ef *ctx)
{
    (void)gpio_pic32mx_init(&ctx->LED1, (struct GPIO_PIC32MX*)ADDR_PORTJ, (size_t)7);
    (void)gpio_pic32mx_init(&ctx->LED2, (struct GPIO_PIC32MX*)ADDR_PORTK, (size_t)7);
    (void)gpio_pic32mx_init(&ctx->LED3, (struct GPIO_PIC32MX*)ADDR_PORTJ, (size_t)3);

    /* turn them off */
    gpio_write(&ctx->LED1, true);
    gpio_write(&ctx->LED2, true);
    gpio_write(&ctx->LED3, true);

    return 0;
}

static int uart_init(/*@partial@*/ struct curiosity_20_pic32mz_ef *ctx)
{
    const struct uart_settings UART_settings = {
        9600ul,
        (size_t)8,
        false,  /* N */
        false,  /* - */
        false,  /* 1 */
    };

    (void)uart_pic32mx_init(&ctx->UART1, (struct UART_PIC32MX*)ADDR_UART1, CLOCK_PIC32MX_PBCLK2);
    (void)uart_setup(&ctx->UART1, &UART_settings);
    (void)uart_pic32mx_init(&ctx->UART6, (struct UART_PIC32MX*)ADDR_UART6, CLOCK_PIC32MX_PBCLK2);
    (void)uart_setup(&ctx->UART6, &UART_settings);

    return 0;
}

static int spi_init(/*@partial@*/ struct curiosity_20_pic32mz_ef *ctx)
{
    struct spi_settings SPI_settings = {
        1000000ul, /* bitrate */
        SPI_MODE_MASTER,
        SPI_CLOCK_MODE_0,
        (size_t)0,  /* ignore frame size */
        SPI_CS_POL_IGNORE,
        (size_t)0,  /* ignore SS */
    };

    (void)spi_pic32mx_init(&ctx->SPI1, (struct SPI_PIC32MX*)ADDR_SPI1, CLOCK_PIC32MX_PBCLK2);
    return spi_setup(&ctx->SPI1, &SPI_settings);
}

static int wd_init(/*@partial@*/ struct curiosity_20_pic32mz_ef *ctx)
{
    struct wd_pic32mx_settings WD_settings = {
        WD_PIC32MX_PERIOD_1MS,
        false, /* !enable_windowed */
    };

    (void)wd_pic32mx_init(&ctx->WDT, (struct WD_PIC32MX*)ADDR_WATCHDOG_TIMER);
    (void)wd_pic32mx_setup(&ctx->WDT, &WD_settings);

    return wd_start(&ctx->WDT);
}

static int pwm_init(/*@partial@*/ struct curiosity_20_pic32mz_ef *ctx)
{
    /* RGB LED */
    struct pwm_pic32mx_settings LED_settings = {
        PWM_PIC32MX_OCTSEL_TIMERX,
        true, /* invert */
    };

    (void)pwm_pic32mx_init(&ctx->LED4.R,
                           (struct PWM_PIC32MX_OC*)ADDR_OC5,
                           (struct PWM_PIC32MX_TC*)ADDR_TIMER2,
                           CLOCK_PIC32MX_PBCLK4);
    (void)pwm_pic32mx_init(&ctx->LED4.G,
                           (struct PWM_PIC32MX_OC*)ADDR_OC8,
                           (struct PWM_PIC32MX_TC*)ADDR_TIMER2,
                           CLOCK_PIC32MX_PBCLK4);
    (void)pwm_pic32mx_init(&ctx->LED4.B,
                           (struct PWM_PIC32MX_OC*)ADDR_OC3,
                           (struct PWM_PIC32MX_TC*)ADDR_TIMER2,
                           CLOCK_PIC32MX_PBCLK4);

    (void)pwm_pic32mx_setup(&ctx->LED4.R, &LED_settings);
    (void)pwm_pic32mx_setup(&ctx->LED4.G, &LED_settings);
    (void)pwm_pic32mx_setup(&ctx->LED4.B, &LED_settings);

    (void)pwm_set_period(&ctx->LED4.R, (pwm_period_us_t)200);
    (void)pwm_set_period(&ctx->LED4.G, (pwm_period_us_t)200);
    (void)pwm_set_period(&ctx->LED4.B, (pwm_period_us_t)200);

    /* EXT */
    struct pwm_pic32mx_settings OC1_settings = {
        PWM_PIC32MX_OCTSEL_TIMERY,
        false, /* invert */
    };

    (void)pwm_pic32mx_init(&ctx->PWM1,
                           (struct PWM_PIC32MX_OC*)ADDR_OC1,
                           (struct PWM_PIC32MX_TC*)ADDR_TIMER3,
                           CLOCK_PIC32MX_PBCLK4);

    (void)pwm_pic32mx_setup(&ctx->PWM1, &OC1_settings);
    (void)pwm_set_period(&ctx->PWM1, (pwm_period_us_t)100);

    return 0;
}

int curiosity_20_pic32mz_ef_init(struct curiosity_20_pic32mz_ef *ctx)
{
    clock_init();
    mux_init();

    (void)gpio_init(ctx);
    (void)uart_init(ctx);
    (void)spi_init(ctx);
    (void)wd_init(ctx);
    (void)pwm_init(ctx);

    return 0;
}
