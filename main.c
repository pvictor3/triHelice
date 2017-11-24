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
#include "sensorlib/mpu6050.h"
#include "sensorlib/hw_mpu6050.h"
#include "utils/uartstdio.h"

#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"

void UARTConfig(void);
uint_fast8_t MPU9250Init(tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr);
uint_fast8_t MPU9250Read(uint_fast8_t ui8Reg, uint8_t *pui8Data, uint_fast16_t ui16Count);
uint_fast8_t MPU9250Write(uint8_t *pui8Data, uint_fast16_t ui16Count);

uint32_t ultimoValorCanal2 = 0, ultimoValorCanal3 = 0, ultimoValorCanal4 = 0, ultimoValorCanal5 = 0;
uint32_t tiempo2, tiempo3, tiempo4, tiempo5;
uint32_t tiempoCanal2, tiempoCanal3, tiempoCanal4, tiempoCanal5;
uint32_t ui32CompDCMStarted;

uint8_t pui8Buffer[6];



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

int main(void)
{
    uint8_t pfData_int[14];
    float pfData[12];
    float *pfAccel, *pfGyro, *pfMag, *pfEulers;
    int i;

    //
    // Initialize convenience pointers that clean up and clarify the code
    // meaning. We want all the data in a single contiguous array so that
    // we can make our pretty printing easier later.
    //
    pfAccel = pfData;
    pfGyro = pfData + 3;
    pfMag = pfData + 6;
    pfEulers = pfData + 9;

    //System clock = 40MHZ
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

	//Activa los periféricos a utilizar
	SYSCTL->RCGC2 |= (1<<5)|(1<<3)|(1<<2)|(1<<0);       //Activa el reloj para GPIOA, GPIOC, GPIOD, GPIOF
	SYSCTL->RCGCPWM |= (1<<1)|(1<<0);                    //Activa el reloj para PWM MODULE 1 y MODULE 0
	SYSCTL->RCGCWTIMER |= (1<<0);                        //Activa el timer 0 de 32bits
	SYSCTL->RCGCI2C |= (1<<1);                           //Activa I2C modulo 1

	//
	// Inicializa las 3 salidas PWM a 8KHz
	//  PF2, PF3, PC4
	pwmInit();

	//
	//Inicializa el GPIOA para generar una interrupcion en ambos flancos
	//
	gpioAConfig();

    //
    // Enable the GPIOA interrupt.
    //
    ROM_IntEnable(INT_GPIOA);

    //
    // Enable the GPIOF interrupt.
    //
    ROM_IntEnable(INT_GPIOF);

    //
    //Inicializa el timer de 32bits para contar microsegundos
    //
    timerConfig();

    //
    //Configura la UART0 para debug
    //
    UARTConfig();
    UARTprintf("\033[H\033[2J");
    UARTprintf("Flight Controller!!! \n");
    UARTprintf("Inicializando perifericos...\n");

    //Configura el I2C
    ROM_GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    ROM_GPIOPinConfigure(GPIO_PA7_I2C1SDA);

    ROM_GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    ROM_GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    I2CMInit(&g_sI2CInst, I2C1_BASE, INT_I2C1, 0xff, 0xff, SysCtlClockGet());

    //MPU6050 EXAMPLE
    for(i = 0; i < 14; i++){
        pfData_int[i] = 3;
    }
    pui8Buffer[0] = 0x6B;
    pui8Buffer[1] = 0x00;
    I2CMWrite(&g_sI2CInst, 0x68, pui8Buffer, 2, I2CMSimpleCallback, 0);
    while(!g_bI2CMSimpleDone);
    g_bI2CMSimpleDone = false;

    //MPU9250Init(&g_sI2CInst, MPU9150_I2C_ADDRESS);

    //MPU9250Write(pui8Buffer, 2); //The first byte is the register addresss

    //
    // Initialize the DCM system. 50 hz sample rate.
    // accel weight = .2, gyro weight = .8, mag weight = .2
    //
    CompDCMInit(&g_sCompDCMInst, 1.0f / 50.0f, 0.2f, 0.6f, 0.2f);

    ui32CompDCMStarted = 0;
    UARTprintf("Inicializacion completa!!!");
	while(1){
	    pui8Buffer[0] = 0x3B;
	        I2CMRead(&g_sI2CInst, 0x68, pui8Buffer, 1, pfData_int, 14,
	                        I2CMSimpleCallback, 0);
	        while(!g_bI2CMSimpleDone);
	        g_bI2CMSimpleDone = false;

	        UARTprintf("acX = %d \n", (pfData_int[0] << 8) | pfData_int[1]);
	        UARTprintf("acY = %d \n", (pfData_int[2] << 8) | pfData_int[3]);
	        UARTprintf("acZ = %d \n", (pfData_int[4] << 8) | pfData_int[5]);
	        UARTprintf("temp = %d \n", (pfData_int[6] << 8) | pfData_int[7]);
	        UARTprintf("gyX = %d \n", (pfData_int[8] << 8) | pfData_int[9]);
	        UARTprintf("gyY = %d \n", (pfData_int[10] << 8) | pfData_int[11]);
	        UARTprintf("gyZ = %d \n", (pfData_int[12] << 8) | pfData_int[13]);



	    //MPU9250Read(0x3B, pfData_int, 12);

        //
        // Get floating point version of the Accel Data in m/s^2.
        //

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
}

//*****************************************************************************
//
//! Writes data to MPU9250 registers.
//!
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the MPU9250.  The first byte of the \e pui8Data buffer contains the value
//! to be written into the \e ui8Reg register, the second value contains the
//! data to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
MPU9250Write(uint8_t *pui8Data,
             uint_fast16_t ui16Count)
{
    //
    // Write the requested registers to the MPU9250.
    //
    if(I2CMWrite(&g_sI2CInst, MPU9150_I2C_ADDRESS,
                  pui8Data, ui16Count, I2CMSimpleCallback, 0) == 0)
    {
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from MPU9250 registers.
//!
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the MPU9250.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
MPU9250Read(uint_fast8_t ui8Reg, uint8_t *pui8Data,
            uint_fast16_t ui16Count)
{
    //
    // Read the requested registers from the MPU9150.
    //
    pui8Buffer[0] = ui8Reg;
    if(I2CMRead(&g_sI2CInst, MPU9150_I2C_ADDRESS,
                pui8Buffer, 1, pui8Data, ui16Count,
                I2CMSimpleCallback, 0) == 0)
    {
        //
        // The I2C write failed return a
        // failure.
        //

        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Initializes the MPU9250 driver.
//!
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the MPU9250 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the MPU9250 driver, preparing it for operation.
//!
//! \return Returns 1 if the MPU9250 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
MPU9250Init(tI2CMInstance *psI2CInst,
            uint_fast8_t ui8I2CAddr)
{
    //
    // Load the buffer with command to perform device reset
    //
    pui8Buffer[0] = MPU9150_O_PWR_MGMT_1;
    pui8Buffer[1] = MPU9150_PWR_MGMT_1_DEVICE_RESET;
    if(I2CMWrite(psI2CInst, ui8I2CAddr,
                 pui8Buffer, 2, I2CMSimpleCallback, 0) == 0)
    {
        return(0);
    }

    //
    // Success
    //
    return(1);
}

void UARTConfig(void){
    //
    // Enable the peripherals used by UART0.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    UARTStdioConfig(0, 115200, 40000000);
}

void IntGPIOAHandler(void){

    uint32_t currentTime = WTIMER0->TAR;

    //
    //Lectura del canal 2
    //
    if( GPIOA->DATA & (1<<2) ){
        if(ultimoValorCanal2 == 0){
            ultimoValorCanal2 = 1;
            tiempo2 = currentTime;
            GPIOA->ICR = (1 << 2);
        }
    }
    else if(ultimoValorCanal2 == 1){
        ultimoValorCanal2 = 0;
        tiempoCanal2 = tiempo2 - currentTime;
        GPIOA->ICR = (1 << 2);
    }

    //
    //Lectura del canal 3
    //
    if( GPIOA->DATA & (1<<3) ){
        if(ultimoValorCanal3 == 0){
            ultimoValorCanal3 = 1;
            tiempo3 = currentTime;
            GPIOA->ICR = (1 << 3);
        }
    }
    else if(ultimoValorCanal3 == 1){
        ultimoValorCanal3 = 0;
        tiempoCanal3 = tiempo3 - currentTime;
        GPIOA->ICR = (1 << 3);
    }

    //
    //Lectura del canal 4
    //
    if( GPIOA->DATA & (1<<4) ){
        if(ultimoValorCanal4 == 0){
            ultimoValorCanal4 = 1;
            tiempo4 = currentTime;
            GPIOA->ICR = (1 << 4);
        }
    }
    else if(ultimoValorCanal4 == 1){
        ultimoValorCanal4 = 0;
        tiempoCanal4 = tiempo4 - currentTime;
        GPIOA->ICR = (1 << 4);
    }

    //
    //Lectura del canal 5
    //
    if( GPIOA->DATA & (1<<5) ){
        if(ultimoValorCanal5 == 0){
            ultimoValorCanal5 = 1;
            tiempo5 = currentTime;
            GPIOA->ICR = (1 << 5);
        }
    }
    else if(ultimoValorCanal5 == 1){
        ultimoValorCanal5 = 0;
        tiempoCanal5 = tiempo5 - currentTime;
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
