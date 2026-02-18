#include <avr/io.h>
#include <util/delay.h>

#define BTN_PIN     PB0
#define PWM_PIN     PB2
#define ADC_PIN     PB1

#define BTN_THRESHOLD 10 /* number of cycles to assume stable press */

typedef enum {
    //MODE_OFF,
    MODE_FULL = 0,
    MODE_PWM,
    //MODE_FADE,
    MODE_END
} Mode_e;

volatile Mode_e mode = MODE_FULL;
volatile uint8_t adc_value = 0x00U;
volatile uint8_t btn_counter = 0x00U; /* button debounce counter */

static void AdcInit(void);
static void PwmInit(void);
static void ButtonInit(void);
static void ModeChange(void);

/* ---------- MAIN ---------- */
int main(void)
{
    AdcInit();
    PwmInit();
    ButtonInit();

    sei(); /* global interrupt enable */

    while (1)
    {
        /* theoretically everything in ISR, at least for now*/
    }
}

/* ---------- ADC ---------- */
static void AdcInit(void)
{
    ADMUX = (1 << MUX0);  /* ADC1 (PB1) */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADIE); /* Enable ADC, prescaler /64, interrupt enable */
    ADCSRA |= (1 << ADSC); /* trigger start of conversion */
}

/* ---------- PWM (Timer0) ---------- */
static void PwmInit(void)
{
    DDRB |= (1 << PWM_PIN); /* PB2 as output */
    TCCR0A = (1 << COM0A1) | (1 << WGM00); /* Fast PWM 8-bit, non-inverting (Mode 5) */
    TCCR0B = (1 << WGM02) | (1 << CS00); /* prescaler 1 */
    OCR0A = 0;  /* start from 0 */
}

/* ---------- Button config ---------- */
static void ButtonInit(void)
{
    DDRB &= ~(1 << BTN_PIN); /* input */
    PORTB |= (1 << BTN_PIN); /* pull-up */
    EICRA = (1 << ISC01); /* ISC01=1, ISC00=0, falling edge */
    EIMSK = (1 << INT0); /* set INT0 */
}

/* ----------- Button reaction ----------*/
static void ModeChange(void)
{
    /* only two modes for now, in future other modes will be added but it requires to dynamically reconfigure Timer */
    /* there is only one timer availible in Attiny10 and it is used for PWM already */
    switch (mode) 
    {
        case MODE_FULL:
            mode = MODE_PWM;
            break;
        case MODE_PWM:
            mode = MODE_FULL;
            break;
        default:
            /* do nothing, to satisfy MISRA */
            break;
    }
}

ISR(ADC_vect)
{
    adc_value = ADCL;
    OCR0A = (mode == 0) ? adc_value : 255; /* set PWM */
    ADCSRA |= (1 << ADSC); /* trigger start of conversion */
}

ISR(INT0_vect) 
{
    if (BTN_THRESHOLD > btn_counter)
    {
        btn_counter++;
    } 
    if (BTN_THRESHOLD <= btn_counter) 
    {
        ModeChange();
        btn_counter = 0;
    }
}
