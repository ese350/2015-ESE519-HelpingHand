/*
 * mbed library program
 *  L3GD20 MEMS motion sensor: 3-axis digital gyroscope, made by STMicroelectronics
 *      http://www.st.com/web/catalog/sense_power/FM89/SC1288
 *          /PF252443?sc=internet/analog/product/252443.jsp
 *  L3G4200D MEMS motion sensor: three-axis digital output gyroscope, made by STMicroelectronics
 *      http://www.st.com/web/catalog/sense_power/FM89/SC1288
 *          /PF250373?sc=internet/analog/product/250373.jsp
 *
 * Copyright (c) 2014,'15 Kenji Arai / JH1PJL
 *  http://www.page.sannet.ne.jp/kenjia/index.html
 *  http://mbed.org/users/kenjiArai/
 *      Created: July      13th, 2014
 *      Revised: Feburary  24th, 2015
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef L3GD20_GYRO_H
#define L3GD20_GYRO_H

#include "mbed.h"

//  L3G4200DMEMS Address
//  7bit address = 0b110100x(0x68 or 0x69 depends on SA0/SDO)
#define L3G4200D_G_CHIP_ADDR (0x68 << 1)    // SA0(=SDO pin) = Ground
#define L3G4200D_V_CHIP_ADDR (0x69 << 1)    // SA0(=SDO pin) = Vdd
//  L3GD20MEMS Address
//  7bit address = 0b110101x(0x6a or 0x6b depends on SA0/SDO)
#define L3GD20_G_CHIP_ADDR   (0x6a << 1)    // SA0(=SDO pin) = Ground
#define L3GD20_V_CHIP_ADDR   (0x6b << 1)    // SA0(=SDO pin) = Vdd

//  L3G4200DMEMS ID
#define I_AM_L3G4200D        0xd3
//  L3GD20MEMS ID
#define I_AM_L3GD20          0xd4
#define I_AM_L3GD20_H        0xd7

//  Register's definition
#define L3GX_WHO_AM_I        0x0f
#define L3GX_CTRL_REG1       0x20
#define L3GX_CTRL_REG2       0x21
#define L3GX_CTRL_REG3       0x22
#define L3GX_CTRL_REG4       0x23
#define L3GX_CTRL_REG5       0x24
#define L3GX_REFERENCE       0x25
#define L3GX_OUT_TEMP        0x26
#define L3GX_STATUS_REG      0x27
#define L3GX_OUT_X_L         0x28
#define L3GX_OUT_X_H         0x29
#define L3GX_OUT_Y_L         0x2a
#define L3GX_OUT_Y_H         0x2b
#define L3GX_OUT_Z_L         0x2c
#define L3GX_OUT_Z_H         0x2d
#define L3GX_FIFO_CTRL_REG   0x2e
#define L3GX_FIFO_SRC_REG    0x2f
#define L3GX_INT1_CFG        0x30
#define L3GX_INT1_SRC        0x31
#define L3GX_INT1_TSH_XH     0x32
#define L3GX_INT1_TSH_XL     0x33
#define L3GX_INT1_TSH_YH     0x34
#define L3GX_INT1_TSH_YL     0x35
#define L3GX_INT1_TSH_ZH     0x36
#define L3GX_INT1_TSH_ZL     0x37
#define L3GX_INT1_DURATION   0x38

// Output Data Rate (ODR)
//      L3G4200DMEMS
#define L3GX_DR_100HZ        0
#define L3GX_DR_200HZ        1
#define L3GX_DR_400HZ        2
#define L3GX_DR_800HZ        3
//      L3GD20MEMS
#define L3GX_DR_95HZ         0
#define L3GX_DR_190HZ        1
#define L3GX_DR_380HZ        2
#define L3GX_DR_760HZ        3

// Bandwidth (Low pass)
#define L3GX_BW_LOW          0
#define L3GX_BW_M_LOW        1
#define L3GX_BW_M_HI         2
#define L3GX_BW_HI           3

// Power-down mode enable/disable
#define L3GX_PD_EN           0
#define L3GX_PD_DIS          1

// Axis control
#define L3GX_X_EN            1
#define L3GX_X_DIS           0
#define L3GX_Y_EN            1
#define L3GX_Y_DIS           0
#define L3GX_Z_EN            1
#define L3GX_Z_DIS           0

// Full Scale
#define L3GX_FS_250DPS       0
#define L3GX_FS_500DPS       1
#define L3GX_FS_2000DPS      2

//Convert from degrees to radians.
#define toRadians(x) (x * 0.01745329252)
//Convert from radians to degrees.
#define toDegrees(x) (x * 57.2957795)

/** Interface for STMicronics MEMS motion sensor: 3-axis digital gyroscope
 *      Chip: L3GD20 (new one) and L3G4200 (old one)
 *
 * @code
 * #include "mbed.h"
 *
 * // I2C Communication
 * L3GX_GYRO gyro(p_sda, p_scl, chip_addr, datarate, bandwidth, fullscale);
 * // If you connected I2C line not only this device but also other devices,
 * //     you need to declare following method.
 * I2C i2c(dp5,dp27);              // SDA, SCL
 * L3GX_GYRO gyro(i2c, chip_addr, datarate, bandwidth, fullscale);
 *
 * int main() {
 * float f[3];
 * int8_t t;
 *
 *   if (gyro.read_id() == I_AM_L3G4200D){
 *      t = gyro.read_temp();
 *      gyro.read_data(f);
 *   }
 * }
 * @endcode
 */

class L3GX_GYRO
{
public:
    /** Configure data pin
      * @param data SDA and SCL pins
      * @param device address L3G4200D(SA0=0 or 1) or L3GD20(SA0=0 or 1)
      * @param ->L3G4200D_G_CHIP_ADDR to L3GD20_V_CHIP_ADDR
      * @param output data rate selection, DR_100HZ/DR_95HZ to DR_800HZ/DR_760HZ
      * @param bandwidth selection, BW_LOW to BW_HI
      * @param full scale selection, FS_250DPS, FS_500DPS, FS_2000DPS
      */
    L3GX_GYRO(PinName p_sda, PinName p_scl,
              uint8_t addr, uint8_t data_rate, uint8_t bandwidth, uint8_t fullscale);

    /** Configure data pin
      * @param data SDA and SCL pins
      * @param device address L3G4200D(SA0=0 or 1) or L3GD20(SA0=0 or 1)
      * @param ->L3G4200D_G_CHIP_ADDR to L3GD20_V_CHIP_ADDR
      * @default output data rate selection = DR_100HZ/DR_95HZ
      * @default bandwidth selection = BW_HI
      * @default full scale selection = FS_250DPS
      */
    L3GX_GYRO(PinName p_sda, PinName p_scl, uint8_t addr);

    /** Configure data pin (with other devices on I2C line)
      * @param I2C previous definition
      * @param other parameters -> please see L3GX_GYRO(PinName p_sda, PinName p_scl,...)
      */
    L3GX_GYRO(I2C& p_i2c,
              uint8_t addr, uint8_t data_rate, uint8_t bandwidth, uint8_t fullscale);

    /** Configure data pin (with other devices on I2C line)
      * @param I2C previous definition
      * @param other parameters -> please see L3GX_GYRO(PinName p_sda, PinName p_scl,...)
      * @default output data rate selection = DR_100HZ/DR_95HZ
      * @default bandwidth selection = BW_HI
      * @default full scale selection = FS_250DPS
      */
    L3GX_GYRO(I2C& p_i2c, uint8_t addr);

    /** Read a tow's complemet type data from Gyro
      * @param none
      * @return temperature unit:degreeC(Celsius)
      */
    int8_t read_temp();

    /** Read a float type data from Gyro
      * @param float type of three arry's address, e.g. float dt_usr[3];
      * @return Gyro motion data unit in param array:dps(degree per second)
      * @return dt_usr[0]->x, dt_usr[1]->y, dt_usr[2]->z
      */
    void read_data(float *dt_usr);

    /** Read a Gyro ID number
      * @param none
      * @return if STM MEMS Gyro, it should be I_AM_L3G4200D(0xd3) or I_AM_L3GD20(0xd4)
      */
    uint8_t read_id();

    /** Read Data Ready flag
      * @param none
      * @return 1 = Ready
      */
    uint8_t data_ready();

    /** Set I2C clock frequency
      * @param freq.
      * @return none
      */
    void frequency(int hz);

    /** Read register (general purpose)
      * @param register's address
      * @return register data
      */
    uint8_t read_reg(uint8_t addr);

    /** Write register (general purpose)
      * @param register's address
      * @param data
      * @return none
      */
    void write_reg(uint8_t addr, uint8_t data);

protected:
    void initialize(uint8_t, uint8_t, uint8_t, uint8_t);

    I2C _i2c;

private:
    float   fs_factor;  // full scale factor
    char    dt[2];      // working buffer
    uint8_t gyro_addr;  // gyro sensor address
    uint8_t gyro_id;    // gyro ID
    uint8_t gyro_ready; // gyro is on I2C line = 1, not = 0
};

#endif      // L3GD20_GYRO_H
