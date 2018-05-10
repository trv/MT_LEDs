#include "adc.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#define NUM_CHANNELS	6
#define NUM_PINS		5

#define GPIOx				GPIOA

#define VBAT_MEAS_EN_PORT	GPIOB
#define VBAT_MEAS_EN_PIN	LL_GPIO_PIN_0

static const uint32_t gpioPins[] = {
	LL_GPIO_PIN_1,
	LL_GPIO_PIN_3,
	LL_GPIO_PIN_4,
	LL_GPIO_PIN_5,
	LL_GPIO_PIN_6,
};

void adc_Init(void)
{
	// clock config
	LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_SYSCLK);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC);

	// enable ADC boost in SYSCFG
	LL_SYSCFG_EnableAnalogBooster();

	LL_ADC_DisableDeepPowerDown(ADC1);
	LL_ADC_EnableInternalRegulator(ADC1);

    LL_GPIO_InitTypeDef gpioConfig;

	// GPIO config for meas_en pin
    LL_GPIO_ResetOutputPin(VBAT_MEAS_EN_PORT, VBAT_MEAS_EN_PIN);

    LL_GPIO_StructInit(&gpioConfig);
    gpioConfig.Pin = VBAT_MEAS_EN_PIN;
    gpioConfig.Speed = LL_GPIO_SPEED_LOW;
    gpioConfig.Mode = LL_GPIO_MODE_OUTPUT;
    gpioConfig.Pull = LL_GPIO_PULL_NO;
    gpioConfig.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(VBAT_MEAS_EN_PORT, &gpioConfig);

    // just leave vbat meas enabled for now since time constant is so slow.
    LL_GPIO_SetOutputPin(VBAT_MEAS_EN_PORT, VBAT_MEAS_EN_PIN);

	// GPIO config for ADC pins

    LL_GPIO_StructInit(&gpioConfig);
    gpioConfig.Speed = LL_GPIO_SPEED_LOW;
    gpioConfig.Mode = LL_GPIO_MODE_ANALOG;
    gpioConfig.Pull = LL_GPIO_PULL_NO;

    for (int i = 0; i < NUM_PINS; i++) {
	    gpioConfig.Pin = gpioPins[i];
	    LL_GPIO_Init(GPIOx, &gpioConfig);
	}

	// ADC Common config (20MHz ADC clock)
	LL_ADC_CommonInitTypeDef adcCommonConfig = {
		.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV4,
	};
	LL_ADC_CommonInit(ADC1_COMMON, &adcCommonConfig);
	
	// ADC config
	LL_ADC_InitTypeDef adcConfig = {
			.Resolution = LL_ADC_RESOLUTION_12B,
			.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT,
			.LowPowerMode = LL_ADC_LP_AUTOWAIT
	};
	LL_ADC_Init(ADC1, &adcConfig);

	LL_ADC_REG_InitTypeDef adcRegConfig = {
		.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE,
		.SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS,
		.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE,
		.ContinuousMode = LL_ADC_REG_CONV_SINGLE,
		.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE,
		.Overrun = LL_ADC_REG_OVR_DATA_OVERWRITTEN,
	};
	LL_ADC_REG_Init(ADC1, &adcRegConfig);

	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_5);
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_6);
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_3, LL_ADC_CHANNEL_8);
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_4, LL_ADC_CHANNEL_9);
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_5, LL_ADC_CHANNEL_10);
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_6, LL_ADC_CHANNEL_11);

	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_5, LL_ADC_SAMPLINGTIME_47CYCLES_5);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_6, LL_ADC_SAMPLINGTIME_47CYCLES_5);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_8, LL_ADC_SAMPLINGTIME_47CYCLES_5);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_9, LL_ADC_SAMPLINGTIME_47CYCLES_5);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_10, LL_ADC_SAMPLINGTIME_47CYCLES_5);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_11, LL_ADC_SAMPLINGTIME_47CYCLES_5);

	LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
	while (LL_ADC_IsCalibrationOnGoing(ADC1));

	LL_ADC_Enable(ADC1);
	while (!LL_ADC_IsActiveFlag_ADRDY(ADC1));

}

void adc_Sample(uint32_t *results)
{
	LL_ADC_REG_StartConversion(ADC1);

	while (!LL_ADC_IsActiveFlag_EOC(ADC1));
	LL_ADC_REG_ReadConversionData32(ADC1);	// read dummy sample

	for (int i = 0; i < NUM_PINS; i++) {
		while (!LL_ADC_IsActiveFlag_EOC(ADC1));
		results[i] = LL_ADC_REG_ReadConversionData32(ADC1);
	}
}

