/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define VOLTAGE_REFERENCE (3.3f)

static uint16_t resultsBuffer[2];


/*
 * ADC Functions not using MAP.
 *
 * For some reason, can't read second channel of ADC when using MAP version of libraries.
 * Problem goes away when not using MAP version, so I'm not. Yes, this is stupid...
 */

int main(void)
{
    /* Halting WDT  */
    MAP_WDT_A_holdTimer();
    MAP_Interrupt_enableSleepOnIsrExit();

    /* Zero-filling buffer */
    memset(resultsBuffer, 0x00, 2); // set two instead of zero b/c want to make sure reset

    /* Setting reference voltage to 2.5  and enabling reference */
    /*REF_A_setReferenceVoltage(REF_A_VREF2_5V);
    REF_A_enableReferenceVoltage();
    */

    /* Initializing ADC (MCLK/1/1) */
    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1,
            0);

    /* Configuring GPIOs for Analog In */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5,
            GPIO_PIN5 | GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);
    /* Set up pins for external voltage reference */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5,
            GPIO_PIN6 | GPIO_PIN7, GPIO_TERTIARY_MODULE_FUNCTION);

    /*  GPIO LED1 for debugging */
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);


    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A0 - A4)  with no repeat)
     * with exterrnal reference
     */
    /* P5.5 - A0
     * P5.4 - A1
     */
    ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, false);
    ADC14_configureConversionMemory(ADC_MEM0,
             ADC_VREFPOS_EXTPOS_VREFNEG_EXTNEG,
             ADC_INPUT_A0, false); // P5.5
    ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_EXTPOS_VREFNEG_EXTNEG,
            ADC_INPUT_A1, false); // P5.4

    /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
     *  is complete and enabling conversions */
    ADC14_enableInterrupt(ADC_INT1); //INT1 -> MEM1

    /* Enabling Interrupts */
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableMaster();

    /* Setting up the sample timer to automatically step through the sequence
     * convert.
     */
    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    /* Triggering the start of the sample */
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();

    /* Going to sleep */
    while (1)
    {

    }
}

float ADC_Convert(uint16_t rawADCValue)
{
    return ((float)rawADCValue/16384)*VOLTAGE_REFERENCE;
    // 2^14 -> 16384
}

/* This interrupt is fired whenever a conversion is completed and placed in
 * ADC_MEM1. This signals the end of conversion and the results array is
 * grabbed and placed in resultsBuffer */
void ADC14_IRQHandler(void)
{
    uint64_t status;
    int i;

    status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    if(status & ADC_INT1)
    {
        ADC14_getMultiSequenceResult(resultsBuffer);

        for(i = 0; i < 2; i++)
        {
            printf("%d %f \n", i, ADC_Convert(resultsBuffer[0]));
        }
        ADC14_toggleConversionTrigger();

    }

}
