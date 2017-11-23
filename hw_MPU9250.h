/*
 * hw_MPU9250.h
 *
 *  Created on: 22/11/2017
 *      Author: pvict
 */

#ifndef HW_MPU9250_H_
#define HW_MPU9250_H_

#define MPU9150_I2C_ADDRESS 0X68

//*****************************************************************************
//
// The following are defines for the MPU9150 register addresses.
//
//*****************************************************************************
#define MPU9150_O_PWR_MGMT_1    0x6B        // Power management 1 register


//*****************************************************************************
//
// The following are defines for the bit fields in the MPU9150_O_PWR_MGMT_1
// register.
//
//*****************************************************************************
#define MPU9150_PWR_MGMT_1_DEVICE_RESET                                       \
                                0x80        // Device reset

//*****************************************************************************
//
// The factors used to convert the acceleration readings from the MPU9150 into
// floating point values in meters per second squared.
//
// Values are obtained by taking the g conversion factors from the data sheet
// and multiplying by 9.81 (1 g = 9.81 m/s^2).
//
//*****************************************************************************
static const float g_fMPU9150AccelFactors[] =
{
    0.0005985482,                           // Range = +/- 2 g (16384 lsb/g)
    0.0011970964,                           // Range = +/- 4 g (8192 lsb/g)
    0.0023941928,                           // Range = +/- 8 g (4096 lsb/g)
    0.0047883855                            // Range = +/- 16 g (2048 lsb/g)
};

//*****************************************************************************
//
// The factors used to convert the acceleration readings from the MPU9150 into
// floating point values in radians per second.
//
// Values are obtained by taking the degree per second conversion factors
// from the data sheet and then converting to radians per sec (1 degree =
// 0.0174532925 radians).
//
//*****************************************************************************
static const float g_fMPU9150GyroFactors[] =
{
    1.3323124e-4,                           // Range = +/- 250 dps (131.0)
    2.6646248e-4,                           // Range = +/- 500 dps (65.5)
    5.3211258e-4,                           // Range = +/- 1000 dps (32.8)
    0.0010642252                            // Range = +/- 2000 dps (16.4)
};

//*****************************************************************************
//
// Converting sensor data to tesla (0.3 uT per LSB)
//
//*****************************************************************************
#define CONVERT_TO_TESLA        0.0000003



#endif /* HW_MPU9250_H_ */
