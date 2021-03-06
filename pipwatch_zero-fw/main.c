#include "main.h"

/* Standard includes. */
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Library includes. */
#include "stm32f10x_it.h"

/* Driver includes. */
#include "STM32_USART.h"
#include "epd.h"
#include "rtclock.h"
#include "btm.h"
#include "battery.h"
#include "leds.h"
#include "buttons.h"
#include "motor.h"
#include "gui.h"
#include "utils.h"


/* The time between cycles of the 'check' task - which depends on whether the
check task has detected an error or not. */
#define mainCHECK_DELAY_NO_ERROR			( ( TickType_t ) 5000 / portTICK_PERIOD_MS )
#define mainCHECK_DELAY_ERROR				( ( TickType_t ) 500 / portTICK_PERIOD_MS )

/* The LED controlled by the 'check' task. */
#define mainCHECK_LED						( 3 )

/* Task priorities. */
#if 0
#define mainSEM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCHECK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )
#define mainFLASH_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainINTEGER_TASK_PRIORITY           ( tskIDLE_PRIORITY )
#define mainGEN_QUEUE_TASK_PRIORITY			( tskIDLE_PRIORITY )
#endif
#define mainECHO_TASK_PRIORITY              ( tskIDLE_PRIORITY + 1 )


/*-----------------------------------------------------------*/

/*
 * Configure the hardware for the demo.
 */
static void prvSetupHardware( void );


/*-----------------------------------------------------------*/


void assert_failed( char *pucFile, unsigned long ulLine );


/*-----------------------------------------------------------*/

int main( void )
{
#ifdef DEBUG
    debug();
#endif

	/* Set up the clocks and memory interface. */
	prvSetupHardware();

    toGuiQueue = xQueueCreate(32, sizeof(struct guievent));

    current_rtime.sec = 0;
    current_rtime.min = 39;
    current_rtime.hour = 21;

    LEDs_Init();
    LEDs_Set(LED0, LED_INTENS_50, LED_INTENS_0, LED_INTENS_0);


	/* Create the 'echo' task, which is also defined within this file. */
    xTaskCreate( BluetoothModemTask, "BTM", 2*configMINIMAL_STACK_SIZE, NULL, mainECHO_TASK_PRIORITY, NULL );

	xTaskCreate( GuiDrawTask, "GuiDraw", 2*configMINIMAL_STACK_SIZE, NULL, mainECHO_TASK_PRIORITY, NULL );

    xTaskCreate( BatteryTask, "Battery", 2*configMINIMAL_STACK_SIZE, NULL, mainECHO_TASK_PRIORITY, NULL );

    xTaskCreate( LEDsTask, "LEDs", 2*configMINIMAL_STACK_SIZE, NULL, mainECHO_TASK_PRIORITY, NULL );

    xTaskCreate( ButtonsTask, "Buttons", 2*configMINIMAL_STACK_SIZE, NULL, mainECHO_TASK_PRIORITY, NULL );

    xTaskCreate( MotorTask, "Motor", 2*configMINIMAL_STACK_SIZE, NULL, mainECHO_TASK_PRIORITY, NULL );


	/* Create the 'check' task, which is also defined within this file. */
	// xTaskCreate( prvCheckTask, "Check", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );

    /* Start the scheduler. */
	vTaskStartScheduler();

    /* Will only get here if there was insufficient memory to create the idle
    task.  The idle task is created within vTaskStartScheduler(). */
	for( ;; );
}

/*-----------------------------------------------------------*/


/* FreeRTOS: User defined function executed from within the idle task. */
void vApplicationIdleHook( void )
{
#if 1
    /* Activate the sleep mode - wait for next interrupt. */
    /* Power modes must be configured first!! */
    __WFI();
#endif
}

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStruct;

#if 1
	/* RCC system reset(for debug purpose). */
	RCC_DeInit ();

#if 1
    /* Enable HSE. */
	RCC_HSEConfig( RCC_HSE_ON );

	/* Wait till HSE is ready. */
	while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET)
        ;
#else
    /* Disable HSE */
    RCC_HSEConfig( RCC_HSE_OFF );
#endif

    /* HCLK = SYSCLK. */
    RCC_HCLKConfig( RCC_SYSCLK_Div1 );          // HCLK=8MHz
	// RCC_HCLKConfig( RCC_SYSCLK_Div2 );     

    /* NOTE: update the constant configCPU_CLOCK_HZ to the correct SysTick (HCLK) operating frequency! */

    /* APB2 max 72MHz. PCLK2  = HCLK. */
    // RCC_PCLK2Config( RCC_HCLK_Div1 );
	RCC_PCLK2Config( RCC_HCLK_Div2 );      // PCLK2=4MHz

    /* APB1 max 36MHz. PCLK1  = HCLK/2. */
    // RCC_PCLK1Config( RCC_HCLK_Div2 );       
    RCC_PCLK1Config( RCC_HCLK_Div4 );    // PCLK1=2MHz

	/* ADCCLK = PCLK2/2. */
	RCC_ADCCLKConfig( RCC_PCLK2_Div2 );    // PCLK2/2 = 2MHz

    /* Flash 2 wait state. */
	*( volatile unsigned long  * )0x40022000 = 0x01;

#if 1
    /* PLLCLK = 8MHz * 9 = 72 MHz */
    // RCC_PLLConfig( RCC_PLLSource_HSE_Div1, RCC_PLLMul_9 );   // 8 * 9 = 72MHz
    // RCC_PLLConfig( RCC_PLLSource_HSI_Div2, RCC_PLLMul_9 );     // 8/2 * 9 = 36 MHz
    // RCC_PLLConfig( RCC_PLLSource_PREDIV1, RCC_PLLMul_9 );     // 8 * 9 = 72 MHz (ok)
    // RCC_PLLConfig( RCC_PLLSource_PREDIV1, RCC_PLLMul_2 );     // 8 * 2 = 16 MHz (ok) - MD_VL
    // RCC_PLLConfig( RCC_PLLSource_HSE_Div1, RCC_PLLMul_2 );      // 8 * 2 = 16 MHz
    // RCC_PLLConfig( RCC_PLLSource_HSI_Div2, RCC_PLLMul_4 );      // 8/2 * 4 = 16 MHz
    RCC_PLLConfig( RCC_PLLSource_HSE_Div2, RCC_PLLMul_2 );      // 8 / 2 * 2 = 8 MHz

    // do { } while (1);
    /* Enable PLL. */
	RCC_PLLCmd( ENABLE );          /* BUG: SysReset here, irrespective if HSI or HSE is used!!! */

	/* Wait till PLL is ready. */
	while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        ;

    /* Select PLL as system clock source. */
    RCC_SYSCLKConfig (RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source. */
    while (RCC_GetSYSCLKSource() != 0x08)
        ;
#endif
#if 0
    /* Select HSI as system clock source. */
    RCC_SYSCLKConfig (RCC_SYSCLKSource_HSI);

    /* Wait till PLL is used as system clock source. */
    while (RCC_GetSYSCLKSource() != 0x00)
        ;
#endif
#if 0
    /* Select HSE-direct as system clock source. */
    RCC_SYSCLKConfig (RCC_SYSCLKSource_HSE);

    /* Wait till PLL is used as system clock source. */
    while (RCC_GetSYSCLKSource() != 0x04)
        ;
#endif



	/* Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks */
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC
							| RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE );

	/* Set the Vector Table base address at 0x08000000. */
	NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0 );

	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

	/* Configure HCLK clock as SysTick clock source. */
	SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );
#endif

    /* Update the SystemCoreClock variable based on current register settings */
    SystemCoreClockUpdate();

    /* --------------------------------------------------------------------------- */

    /* Unused pins - pull up */
    /* GPIOA: PA10 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* GPIOB: PB12 PB14 PB15 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    /* GPIOC: PC0 PC5 PC6 PC7 PC8 PC9 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* ------------------------------------------------------------------------ */

    /* Init SPI and GPIO for EPD */
    epdInitInterface();

	/* APB1 Periph clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE );

    /* --------------------------------------------------------------------------- */
    /* RTC */
    /* Enable access to backup registers and RTC */
    PWR_BackupAccessCmd(ENABLE);

    /* Reset backup domain; this allows one-time configuration of RTC clock below. */
    BKP_DeInit();

    /* Enable LSE extenal xtal osci */
    RCC_LSEConfig(RCC_LSE_ON);
    
    /* FIXME: LSE SOMETIMES STOPS WORKING !! - TEMPERATURE ?? */
#if 0
    /* wait until LSE xtal osci is stable */
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) { }
    /* Set RTC clock source = external 32.768kHz xtal */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#else

    RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div128);

#endif

    /* Enable RTC clock after it has been selected */
    RCC_RTCCLKCmd(ENABLE);

    RTC_ExitConfigMode();
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();

#if 0
    /* set prescaler: divide 32.768kHz LSE XTAL clock by 32768 -> generate 1Hz irq */
    RTC_SetPrescaler(32768);
#else
    /* set prescaler: divide 8MHz/128 HSE clock by 62500 -> generate 1Hz irq */
    RTC_SetPrescaler(62500);
#endif

    RTC_WaitForLastTask();

    /* enable RTC irq */
    RTC_ITConfig(RTC_IT_SEC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_Init( &NVIC_InitStructure );

    RTC_WaitForLastTask();

    /* ------------------------------------------------------------------------ */
    /* ADC */
#if 1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_Init(ADC1, &ADC_InitStruct);

    ADC_Cmd(ADC1, ENABLE);
    ADC_TempSensorVrefintCmd(ENABLE);

    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
    NVIC_Init( &NVIC_InitStructure );
#endif

    /* ------------------------------------------------------------------------ */
    /* Buttons */
    /* Configure GPIO pins : PB5,6,7 = Buttons 0, 1, 2 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USBPOW (PA9) input pull-down */
    GPIO_InitStruct.GPIO_Pin = USBPOW_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(USBPOW_Port, &GPIO_InitStruct);

    /* CHARGESTAT (PB11) input pull-up */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* configure interrupts from buttons: EXTI 5, 6, 7  */
    EXTI_InitTypeDef EXTI_InitStruct;
    EXTI_InitStruct.EXTI_Line = EXTI_Line5 | EXTI_Line6 | EXTI_Line7 /*| EXTI_Line9*/;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStruct);

    /* set EXTI sources */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource5); // BTN0
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6); // BTN1
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7); // BTN2
    // GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource9); // USBPOW

#if 1
    /* all the interrupts go to the EXTI9_5_IRQn IRQ-line */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_Init( &NVIC_InitStructure );
#endif
    
    /* ------------------------------------------------------------------------ */
    /* LEDs */
    /* Configure GPIO pin : PC10=LEDRK, PC11=LEDGK, PC12=LEDBK Output */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    /* set cathodes HI */
    GPIO_SetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);

    /* Configure GPIO pin : PD2=LED1A,  Output */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOD, &GPIO_InitStruct);
    /* set anode LO */
    GPIO_ResetBits(GPIOD, GPIO_Pin_2);

    /* Configure GPIO pin : PB8=LED2A, PB9=LED3A, Output */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    /* set anode LO */
    GPIO_ResetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);

    /* The CHARGEBLUE LED (PA8): Set high-impedence floating -> no current  */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);



    /* ------------------------------------------------------------------------ */
    /* Motors */
    /* Configure GPIO pin : PB13 Output */
    GPIO_InitStruct.GPIO_Pin = MOTOR_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(MOTOR_Port, &GPIO_InitStruct);
    /* set Motor_1 off */
    GPIO_ResetBits(MOTOR_Port, MOTOR_Pin);


    /* ------------------------------------------------------------------------ */
    /* BTM GPIO Signals */
#if 0
    /* Configure GPIO pin : PC4=BTM_RESET,  Output */
    GPIO_InitStruct.GPIO_Pin = BTM_Reset_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(BTM_Reset_Port, &GPIO_InitStruct);
    GPIO_SetBits(BTM_Reset_Port, BTM_Reset_Pin);
#endif

    /* ------------------------------------------------------------------------ */
    /* Debugging in low-power modes */
    DBGMCU_Config(DBGMCU_SLEEP | DBGMCU_STOP | DBGMCU_STANDBY, ENABLE);
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	/* This function will get called if a task overflows its stack.   If the
	parameters are corrupt then inspect pxCurrentTCB to find which was the
	offending task. */

	( void ) pxTask;
	( void ) pcTaskName;

	for( ;; );
}
/*-----------------------------------------------------------*/

void assert_failed( char *pucFile, unsigned long ulLine )
{
	( void ) pucFile;
	( void ) ulLine;

    char buf[64];

    strcpy(buf, "ASS-FAIL: ");
    strcat(buf, pucFile);
    strcat(buf, ":");
    itostr(buf+strlen(buf), 8, ulLine);
    strcat(buf, ";");

    printstr(buf);

	for( ;; );
}

/*-----------------------------------------------------------*/

