/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
 * Author "Romain Forlot" based upon work of Mark Williams
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



// This has to be change depends on i2cdetect 
// returned addresses
#define MAG_ADDRESS            0x1E
#define ACC_ADDRESS            0x1E
#define GYR_ADDRESS            0x6A

/** LSM9DS0 Gyro Registers **/
#define WHO_AM_I_G          0x0F
#define CTRL_REG1_G         0x20
#define CTRL_REG2_G         0x21
#define CTRL_REG3_G         0x22
#define CTRL_REG4_G         0x23
#define CTRL_REG5_G         0x24
#define REFERENCE_G         0x25
#define STATUS_REG_G        0x27
#define OUT_X_L_G           0x28
#define OUT_X_H_G           0x29
#define OUT_Y_L_G           0x2A
#define OUT_Y_H_G           0x2B
#define OUT_Z_L_G           0x2C
#define OUT_Z_H_G           0x2D
#define FIFO_CTRL_REG_G     0x2E
#define FIFO_SRC_REG_G      0x2F
#define INT1_CFG_G          0x30
#define INT1_SRC_G          0x31
#define INT1_THS_XH_G       0x32
#define INT1_THS_XL_G       0x33
#define INT1_THS_YH_G       0x34
#define INT1_THS_YL_G       0x35
#define INT1_THS_ZH_G       0x36
#define INT1_THS_ZL_G       0x37
#define INT1_DURATION_G     0x38

//////////////////////////////////////////
// LSM9DS0 Accel/Magneto (XM) Registers //
//////////////////////////////////////////
#define OUT_TEMP_L_XM       0x05
#define OUT_TEMP_H_XM       0x06
#define STATUS_REG_M        0x07
#define OUT_X_L_M           0x08
#define OUT_X_H_M           0x09
#define OUT_Y_L_M           0x0A
#define OUT_Y_H_M           0x0B
#define OUT_Z_L_M           0x0C
#define OUT_Z_H_M           0x0D
#define WHO_AM_I_XM         0x0F
#define INT_CTRL_REG_M      0x12
#define INT_SRC_REG_M       0x13
#define INT_THS_L_M         0x14
#define INT_THS_H_M         0x15
#define OFFSET_X_L_M        0x16
#define OFFSET_X_H_M        0x17
#define OFFSET_Y_L_M        0x18
#define OFFSET_Y_H_M        0x19
#define OFFSET_Z_L_M        0x1A
#define OFFSET_Z_H_M        0x1B
#define REFERENCE_X         0x1C
#define REFERENCE_Y         0x1D
#define REFERENCE_Z         0x1E
#define CTRL_REG0_XM        0x1F
#define CTRL_REG1_XM        0x20
#define CTRL_REG2_XM        0x21
#define CTRL_REG3_XM        0x22
#define CTRL_REG4_XM        0x23
#define CTRL_REG5_XM        0x24
#define CTRL_REG6_XM        0x25
#define CTRL_REG7_XM        0x26
#define STATUS_REG_A        0x27
#define OUT_X_L_A           0x28
#define OUT_X_H_A           0x29
#define OUT_Y_L_A           0x2A
#define OUT_Y_H_A           0x2B
#define OUT_Z_L_A           0x2C
#define OUT_Z_H_A           0x2D
#define FIFO_CTRL_REG       0x2E
#define FIFO_SRC_REG        0x2F
#define INT_GEN_1_REG       0x30
#define INT_GEN_1_SRC       0x31
#define INT_GEN_1_THS       0x32
#define INT_GEN_1_DURATION  0x33
#define INT_GEN_2_REG       0x34
#define INT_GEN_2_SRC       0x35
#define INT_GEN_2_THS       0x36
#define INT_GEN_2_DURATION  0x37
#define CLICK_CFG           0x38
#define CLICK_SRC           0x39
#define CLICK_THS           0x3A
#define TIME_LIMIT          0x3B
#define TIME_LATENCY        0x3C
#define TIME_WINDOW         0x3D

/*
 * Will read from an i2c char device block data
 * 
 * parameters of readBlock are:
 *   command : unsigned 8 bits integer : register address to read
 * 	 size : unsigned 8 bits integer  : bytes to read
 * 	 data : unsigned 8 bits integer : pointer use to retrieve data read 
 */
void readBlock(uint8_t command, uint8_t size, uint8_t *data);

/*
 * Select i2c device
 * 
 * parameters of selectDevice are:
 *   file : integer : 
 * 	 addr : integer  : 
 * 
 */
void selectDevice(int file, int addr);


void readACC(int  *a);
void readMAG(int  *m);
void readGYR(int *g);
void writeAccReg(uint8_t reg, uint8_t value);
void writeMagReg(uint8_t reg, uint8_t value);
void writeGyrReg(uint8_t reg, uint8_t value);

/*
 * Enable IMU sensors
 *
 * Accelerometer configuration enabled : 
 * 	z,y,x axis enabled, continuos update,  100Hz data rate
 * 	Sensitivity : +/- 16G full scale
 * 
 * Magnetometer configuration enabled :
 * 	Temp enable, M data rate = 50Hz
 * 	Sensitivity : +/-12gauss
 * 	Continuous-conversion mode
 * 
 * Gyroscope configuration enabled :
 * 	Normal power mode, all axes enabled
 * 	Continuos update, 2000 dps full scale
 * 
 */
void enableIMU();