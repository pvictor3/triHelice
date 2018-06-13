#include <stdint.h>
#include <stdbool.h>

#include "CMSIS.h"
#include "drivers/pwm.h"
#include "drivers/gpioAReceiver.h"
#include "drivers/timer.h"
#include "hw_MPU9250.h"

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/comp_dcm.h"
#include "utils/uartstdio.h"
#include "utils/scheduler.h"

#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"

#define MPU9250_ADDRESS 0x68
#define MAG_ADDRESS     0x0C

void UARTConfig(void);

uint32_t ultimoValorCanal1=0, ultimoValorCanal2 = 0, ultimoValorCanal3 = 0, ultimoValorCanal4 = 0, ultimoValorCanal5 = 0;
uint32_t tiempo1, tiempo2, tiempo3, tiempo4, tiempo5;
uint32_t tiempoCanal1, tiempoCanal2, tiempoCanal3, tiempoCanal4, tiempoCanal5;
uint32_t ui32CompDCMStarted;

uint8_t pui8Buffer[6];

uint32_t g_ui32SchedulerNumTasks = 4;

//*****************************************************************************
//
// Definition of the system tick rate. This results in a tick period of 10mS.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// Prototypes of functions which will be called by the scheduler.
//
//*****************************************************************************
static void updateDCM(void *pvParam);
static void printData(void *pvParam);
static void readSensors(void *pvParam);
static void PIDAngulo(void *pvParam);

//*****************************************************************************
//
// This table defines all the tasks that the scheduler is to run, the periods
// between calls to those tasks, and the parameter to pass to the task.
//
//*****************************************************************************
tSchedulerTask g_psSchedulerTable[] =
{
{ readSensors, (void *)0, 1, 0, true},
{ updateDCM, (void *)0, 2, 0, true},
{ printData, (void *)0, 50, 0, true},
{ PIDAngulo, (void *)0, 10, 0, true},
};

//*****************************************************************************
//
// The number of entries in the global scheduler task table.
//
//*****************************************************************************
unsigned long g_ulSchedulerNumTasks = (sizeof(g_psSchedulerTable) /
sizeof(tSchedulerTask));

//*****************************************************************************
//
// Global instance structure for the I2C master driver.
//
//*****************************************************************************
tI2CMInstance g_sI2CInst;

//*****************************************************************************
//
// Global Instance structure to manage the DCM state.
//
//*****************************************************************************
tCompDCM g_sCompDCMInst;

//
// A boolean that is set when an I2C transaction is completed.
//
volatile bool g_bI2CMSimpleDone = false;

//
// The function that is provided by this example as a callback when I2C
// transactions have completed.
//
void
I2CMSimpleCallback(void *pvData, uint_fast8_t ui8Status)
{
//
// See if an error occurred.
//
if(ui8Status != I2CM_STATUS_SUCCESS)
{
//
// An error occurred, so handle it here if required.
//
}
//
// Indicate that the I2C transaction has completed.
//
g_bI2CMSimpleDone = true;
}
uint8_t pfData_int[14];
uint8_t pfData_int_Mag[6];

float pfData[12];
float *pfAccel, *pfGyro, *pfMag, *pfEulers;
int i;
float angle;

int32_t e[3], u[2];
int32_t q0,q1,q2;
uint32_t kp = 1, ti = 0, td = 0;
uint32_t T = 0.1;

int32_t ev[3], uv[2];
int32_t qv0,qv1,qv2;
uint32_t kpv = 1, tiv = 0, tdv = 0;

int main(void)
{
    //
    // Initialize convenience pointers that clean up and clarify the code
    // meaning. We want all the data in a single contiguous array so that
    // we can make our pretty printing easier later.
    //
    pfAccel = pfData;
    pfGyro = pfData + 3;
    pfMag = pfData + 6;
    pfEulers = pfData + 9;

    u[0] = 0;
    e[0] = 0;
    e[1] = 0;
    //q0 = kp[1 + T/2ti + td/T]
    //q1 = -kp[1 - T/2ti + 2td/T]
    //q2 = kptd/T
    q0 = kp * (1 + T / (2 * ti) + td / T);
    q1 = -kp * (1 - T / (2 * ti) + (2 * td) / T);
    q2 = (kp * td) / T;

    uv[0] = 0;
    ev[0] = 0;
    ev[1] = 0;
    //q0 = kp[1 + T/2ti + td/T]
    //q1 = -kp[1 - T/2ti + 2td/T]
    //q2 = kptd/T
    qv0 = kpv * (1 + T / (2 * tiv) + tdv / T);
    qv1 = -kpv * (1 - T / (2 * tiv) + (2 * tdv) / T);
    qv2 = (kpv * tdv) / T;

    //System clock = 40MHZ
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

	//
	// Initialize the task scheduler and configure the SysTick to interrupt
	// 100 times per second(Every 10ms).
	//
	SchedulerInit(TICKS_PER_SECOND);

	//Activa los periféricos a utilizar
	SYSCTL->RCGC2 |= (1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0);        //Activa el reloj para GPIOA, GPIOB, GPIOC, GPIOD, GPIOE,GPIOF
	SYSCTL->RCGCPWM |= (1<<0);                                  //Activa el reloj para PWM MODULE 0
	SYSCTL->RCGCWTIMER |= (1<<0);                               //Activa el timer 0 de 32bits
	SYSCTL->RCGCI2C |= (1<<1);                                  //Activa I2C modulo 1

	//
	//Configura la UART0 para debug
	//
	UARTConfig();
	UARTprintf("\033[H\033[2J");
	UARTprintf("Flight Controller!!! \n");
	UARTprintf("Inicializando perifericos...\n");

	//
	// Inicializa las 3 salidas PWM0 a 50Hz
	//  PE5, PC4, PC5
	pwmInit();
	UARTprintf("PWM Iniciado \n");

	//
	//Inicializa el GPIOA para generar una interrupcion en ambos flancos y leer el receiver
	//
	gpioAConfig();
	UARTprintf("GPIOA Iniciado \n");

    //
    //Inicializa el timer de 32bits para contar microsegundos
    //
    timerConfig();
    UARTprintf("Timer Iniciado \n");

    //Configura el I2C
    ROM_GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    ROM_GPIOPinConfigure(GPIO_PA7_I2C1SDA);

    ROM_GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    ROM_GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    I2CMInit(&g_sI2CInst, I2C1_BASE, INT_I2C1, 0xff, 0xff, SysCtlClockGet());
    UARTprintf("I2C Iniciado \n");

    //MPU9250 Reset
    pui8Buffer[0] = 0x6B;   //PWR_MGMT_1 address
    pui8Buffer[1] = 0x80;   //Reset
    I2CMWrite(&g_sI2CInst, MPU9250_ADDRESS, pui8Buffer, 2, I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;
    UARTprintf("Acelerometro y Giroscopio Iniciado \n");

    //Magnetometro Mode 3 (100Hz)
    pui8Buffer[0] = 0x37;   //INT_PIN_CFG
    pui8Buffer[1] = 0x02;   //BYPASS_EN
    I2CMWrite(&g_sI2CInst, MPU9250_ADDRESS, pui8Buffer, 2, I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;

    pui8Buffer[0] = 0x0A;   //CNTL1
    pui8Buffer[1] = 0x06;   //Continous measurement mode
    I2CMWrite(&g_sI2CInst, MAG_ADDRESS, pui8Buffer, 2, I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;
    UARTprintf("Magnetometro iniciado \n");

    pui8Buffer[0] = 0x75;   //WHO AM I
    I2CMRead(&g_sI2CInst, MPU9250_ADDRESS, pui8Buffer, 1, pfData_int, 1,
             I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;
    UARTprintf("Yo soy %X \n", pfData_int[0]);

    if(pfData_int[0] != 0x71) return 0;

    pui8Buffer[0] = 0x00;   //WHO AM I
    I2CMRead(&g_sI2CInst, MAG_ADDRESS, pui8Buffer, 1, pfData_int, 1,
             I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;
    UARTprintf("Yo soy %X \n", pfData_int[0]);

    if(pfData_int[0] != 0x48) return 0;

    //
    // Initialize the DCM system. 50 hz sample rate.
    // accel weight = .2, gyro weight = .8, mag weight = .2
    //
    CompDCMInit(&g_sCompDCMInst, 1.0f / 50.0f, 0.2f, 0.6f, 0.2f);
    ui32CompDCMStarted = 0;
    UARTprintf("DCM Iniciado \n");

    //Activa las tareas a ejecutar por el scheduler
    //
    SchedulerTaskEnable(0,true);
    SchedulerTaskEnable(1,true);
    SchedulerTaskEnable(2,true);
    SchedulerTaskEnable(3,true);
    UARTprintf("Scheduler Iniciado \n");


    UARTprintf("Inicializacion completa!!! \n");
    UARTprintf("Tiempo: %dms", SchedulerTickCountGet()*10);
    //
    // Enable the GPIOA interrupt.
    //
    ROM_IntEnable(INT_GPIOA);

    //
    // Enable the GPIOF interrupt.
    //
    ROM_IntEnable(INT_GPIOF);
	while(1){
	    //
	    // Tell the scheduler to call any periodic tasks that are due to be
	    // called.
	    //
	    SchedulerRun();
	}
}

static void PIDAngulo(void *pvParam){
    //PID ANGULO
    //*********ki = kp/ti
    //*********kd = kp*td
    //q0 = kp[1 + T/2ti + td/T]
    //q1 = -kp[1 - T/2ti + 2td/T]
    //q2 = kptd/T
    //u[k] = u[k-1] + q0 * e[k] +q1 * e[k-1] + q2 * e[k-2];
    e[2] = 0 - pfEulers[0];
    u[1] = u[0] + q0 * e[2] + q1 * e[1] + q2 * e[0];

    u[0] = u[1];
    e[0] = e[1];
    e[1] = e[2];

    //PID VELOCIDAD
    ev[2] = u[1] - pfGyro[0];
    uv[1] = uv[0] + qv0 * ev[2] + qv1 * ev[1] + qv2 * ev[0];

    uv[0] = uv[1];
    ev[0] = ev[1];
    ev[1] = ev[2];
}

static void readSensors(void *pvParam){
    //Read sensors data Acel and Gyro
    pui8Buffer[0] = 0x3B;   //ACCEL_XOUT_H
    I2CMRead(&g_sI2CInst, MPU9250_ADDRESS, pui8Buffer, 1, pfData_int, 14,
             I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;

    pui8Buffer[0] = 0x03;
    I2CMRead(&g_sI2CInst, MAG_ADDRESS, pui8Buffer, 1, pfData_int_Mag, 6,
             I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;

    uint8_t status2;
    pui8Buffer[0] = 0x09;
    I2CMRead(&g_sI2CInst, MAG_ADDRESS, pui8Buffer, 1, &status2, 1,
             I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;

    pfAccel[0] = (int16_t)((pfData_int[0]<<8)|pfData_int[1]) * 0.0005985482;
    pfAccel[1] = (int16_t)((pfData_int[2]<<8)|pfData_int[3]) * 0.0005985482;
    pfAccel[2] = (int16_t)((pfData_int[4]<<8)|pfData_int[5]) * 0.0005985482;

    pfGyro[0] = (int16_t)((pfData_int[8]<<8)|pfData_int[9]) * 1.3323124e-4;
    pfGyro[1] = (int16_t)((pfData_int[10]<<8)|pfData_int[11]) * 1.3323124e-4;
    pfGyro[2] = (int16_t)((pfData_int[12]<<8)|pfData_int[13]) * 1.3323124e-4;

    pfMag[0] = (int16_t)((pfData_int_Mag[1]<<8)|pfData_int_Mag[0]) * 0.0000003;
    pfMag[1] = (int16_t)((pfData_int_Mag[3]<<8)|pfData_int_Mag[2]) * 0.0000003;
    pfMag[2] = (int16_t)((pfData_int_Mag[5]<<8)|pfData_int_Mag[4]) * 0.0000003;
}

static void printData(void *pvParam){
    uint32_t ui32Idx;
    int32_t i32IPart[12];
    int32_t i32FPart[12];
    /*pfMag[0] *= 1e6;
    pfMag[1] *= 1e6;
    pfMag[2] *= 1e6;*/
    //
    // Get Euler data. (Roll Pitch Yaw)
    //
    CompDCMComputeEulers(&g_sCompDCMInst, pfEulers, pfEulers + 1,
                         pfEulers + 2);

    //
    // Convert Eulers to degrees. 180/PI = 57.29...
    // Convert Yaw to 0 to 360 to approximate compass headings.
    //
    pfEulers[0] *= 57.295779513082320876798154814105f;
    pfEulers[1] *= 57.295779513082320876798154814105f;
    pfEulers[2] *= 57.295779513082320876798154814105f;
    if(pfEulers[2] < 0)
    {
        pfEulers[2] += 360.0f;
    }
    for(ui32Idx = 0; ui32Idx < 12; ui32Idx++)
                {
                    //
                    // Conver float value to a integer truncating the decimal part.
                    //
                    i32IPart[ui32Idx] = (int32_t) pfData[ui32Idx];

                    //
                    // Multiply by 1000 to preserve first three decimal values.
                    // Truncates at the 3rd decimal place.
                    //
                    i32FPart[ui32Idx] = (int32_t) (pfData[ui32Idx] * 1000.0f);

                    //
                    // Subtract off the integer part from this newly formed decimal
                    // part.
                    //
                    i32FPart[ui32Idx] = i32FPart[ui32Idx] -
                                        (i32IPart[ui32Idx] * 1000);

                    //
                    // make the decimal part a positive number for display.
                    //
                    if(i32FPart[ui32Idx] < 0)
                    {
                        i32FPart[ui32Idx] *= -1;
                    }
                }
    //UARTprintf("Acel %3d.%03d   %3d.%03d   %3d.%03d\n", i32IPart[0], i32FPart[0], i32IPart[1], i32FPart[1], i32IPart[2], i32FPart[2]);
    /*UARTprintf("Gyro %3d.%03d   %3d.%03d   %3d.%03d\n", i32IPart[3], i32FPart[3], i32IPart[4], i32FPart[4], i32IPart[5], i32FPart[5]);
    //UARTprintf("Magn %3d.%03d   %3d.%03d   %3d.%03d\n", i32IPart[6], i32FPart[6], i32IPart[7], i32FPart[7], i32IPart[8], i32FPart[8]);
    UARTprintf("Angulos %3d.%03d,%3d.%03d,%3d.%03d\n", i32IPart[9], i32FPart[9], i32IPart[10], i32FPart[10], i32IPart[11], i32FPart[11]);
    //UARTprintf("Tiempo: %dms \n", SchedulerTickCountGet());
    //UARTprintf("Canal1= %d Canal2= %d Canal3= %d Canal4= %d Canal5= %d \n", tiempoCanal1, tiempoCanal2, tiempoCanal3, tiempoCanal4, tiempoCanal5);
    UARTprintf("Error = %d  Control = %d \n", e[2], u[1]);
    UARTprintf("Error2 = %d Control2 = %d \n", ev[2], uv[1]);*/
    UARTprintf("Canal1 = %d\nCanal2 = %d\nCanal3 = %d\nCanal4 = %d\nCanal5 = %d\n", tiempoCanal1, tiempoCanal2, tiempoCanal3, tiempoCanal4, tiempoCanal5);
}

static void updateDCM(void *pvParam){
    //
    // Check if this is our first data ever.
    //
    if(ui32CompDCMStarted == 0)
    {
        //
        // Set flag indicating that DCM is started.
        // Perform the seeding of the DCM with the first data set.
        //
        ui32CompDCMStarted = 1;
        CompDCMMagnetoUpdate(&g_sCompDCMInst, pfMag[0], pfMag[1],
                             pfMag[2]);
        CompDCMAccelUpdate(&g_sCompDCMInst, pfAccel[0], pfAccel[1],
                           pfAccel[2]);
        CompDCMGyroUpdate(&g_sCompDCMInst, pfGyro[0], pfGyro[1],
                          pfGyro[2]);
        CompDCMStart(&g_sCompDCMInst);
    }
    else
    {
        //
        // DCM Is already started.  Perform the incremental update.
        //
        CompDCMMagnetoUpdate(&g_sCompDCMInst, pfMag[0], pfMag[1],
                             pfMag[2]);
        CompDCMAccelUpdate(&g_sCompDCMInst, pfAccel[0], pfAccel[1],
                           pfAccel[2]);
        CompDCMGyroUpdate(&g_sCompDCMInst, -pfGyro[0], -pfGyro[1],
                          pfGyro[2]);
        CompDCMUpdate(&g_sCompDCMInst);
    }
}

void UARTConfig(void){
    //
    // Enable the peripherals used by UART1.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
    ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 57600, 8-N-1 operation.
    //
    UARTStdioConfig(1, 57600, 40000000);
}
void IntGPIOFHandler(void){
    uint32_t currentTime = WTIMER0->TAR;
    //
    //Lectura GPIOF4
    //
    if( GPIOF->DATA & (1<<4) ){
        if(ultimoValorCanal5 == 0){
            ultimoValorCanal5 = 1;
            tiempo5 = currentTime;
            GPIOF->ICR = (1 << 4);
        }
    }
    else if(ultimoValorCanal5 == 1){
        ultimoValorCanal5 = 0;
        tiempoCanal5 = tiempo5 - currentTime;
        GPIOF->ICR = (1 << 4);
    }
    GPIOF->ICR = (1 << 4);
}

void IntGPIOAHandler(void){

    uint32_t currentTime = WTIMER0->TAR;

    //
    //Lectura GPIOA2
    //
    if( GPIOA->DATA & (1<<2) ){
        if(ultimoValorCanal1 == 0){
            ultimoValorCanal1 = 1;
            tiempo1 = currentTime;
            GPIOA->ICR = (1 << 2);
        }
    }
    else if(ultimoValorCanal1 == 1){
        ultimoValorCanal1 = 0;
        tiempoCanal1 = tiempo1 - currentTime;
        GPIOA->ICR = (1 << 2);
    }

    //
    //Lectura GPIOA3
    //
    if( GPIOA->DATA & (1<<3) ){
        if(ultimoValorCanal2 == 0){
            ultimoValorCanal2 = 1;
            tiempo2 = currentTime;
            GPIOA->ICR = (1 << 3);
        }
    }
    else if(ultimoValorCanal2 == 1){
        ultimoValorCanal2 = 0;
        tiempoCanal2 = tiempo2 - currentTime;
        GPIOA->ICR = (1 << 3);
    }

    //
    //Lectura GPIOA4
    //
    if( GPIOA->DATA & (1<<4) ){
        if(ultimoValorCanal3 == 0){
            ultimoValorCanal3 = 1;
            tiempo3 = currentTime;
            GPIOA->ICR = (1 << 4);
        }
    }
    else if(ultimoValorCanal3 == 1){
        ultimoValorCanal3 = 0;
        tiempoCanal3 = tiempo3 - currentTime;
        GPIOA->ICR = (1 << 4);
    }

    //
    //Lectura GPIO5
    //
    if( GPIOA->DATA & (1<<5) ){
        if(ultimoValorCanal4 == 0){
            ultimoValorCanal4 = 1;
            tiempo4 = currentTime;
            GPIOA->ICR = (1 << 5);
        }
    }
    else if(ultimoValorCanal4 == 1){
        ultimoValorCanal4 = 0;
        tiempoCanal4 = tiempo4 - currentTime;
        GPIOA->ICR = (1 << 5);
    }
}

//
// The interrupt handler for the I2C module.
//
void
I2CMSimpleIntHandler(void)
{
//
// Call the I2C master driver interrupt handler.
//
I2CMIntHandler(&g_sI2CInst);
}
