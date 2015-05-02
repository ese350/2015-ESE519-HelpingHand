/*
 * mbed library program
 *  L3GD20 MEMS motion sensor: 3-axis digital gyroscope, made by STMicroelectronics
 *      http://www.st.com/web/catalog/sense_power/FM89/SC1288
 *          /PF252443?sc=internet/analog/product/252443.jsp
 *  L3G4200 DMEMS motion sensor: three-axis digital output gyroscope, made by STMicroelectronics
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

#include "L3GD20_YY.h"


L3GX_GYRO::L3GX_GYRO (PinName p_sda, PinName p_scl,
                      uint8_t addr, uint8_t data_rate, uint8_t bandwidth, uint8_t fullscale) : _i2c(p_sda, p_scl)
{
    _i2c.frequency(400000);
    initialize (addr, data_rate, bandwidth, fullscale);
}

L3GX_GYRO::L3GX_GYRO (PinName p_sda, PinName p_scl, uint8_t addr) : _i2c(p_sda, p_scl)
{
    _i2c.frequency(400000);
    initialize (addr, L3GX_DR_95HZ, L3GX_BW_HI, L3GX_FS_250DPS);
}

L3GX_GYRO::L3GX_GYRO (I2C& p_i2c,
                      uint8_t addr, uint8_t data_rate, uint8_t bandwidth, uint8_t fullscale) : _i2c(p_i2c)
{
    _i2c.frequency(400000);
    initialize (addr, data_rate, bandwidth, fullscale);
}

L3GX_GYRO::L3GX_GYRO (I2C& p_i2c, uint8_t addr) : _i2c(p_i2c)
{
    _i2c.frequency(400000);
    initialize (addr, L3GX_DR_95HZ, L3GX_BW_HI, L3GX_FS_250DPS);
}

void L3GX_GYRO::initialize (uint8_t addr, uint8_t data_rate, uint8_t bandwidth, uint8_t fullscale)
{
    // Check gyro is available of not
    gyro_addr = addr;
    dt[0] = L3GX_WHO_AM_I;//dt[0] is set to 0xf
    //wait_ms(100);
    dt[1] = L3GX_WHO_AM_I;//dt[1] os set to 0xf
    //wait_ms(100);
    //printf("after assign:%x;%x\r\n",dt[0],dt[1]);
    _i2c.write(gyro_addr, dt, 1, true); //this does not change dt[0] or dt[1]
    //printf("after write:%x;%x\r\n",dt[0],dt[1]);
    _i2c.read(gyro_addr, dt, 1, false);//dt[0] is set to d7,dt[1] remains same
    //printf("after read:%x;%x\r\n",dt[0],dt[1]);
    if (dt[0] == I_AM_L3G4200D) {
        gyro_ready = 1;
    } else if (dt[0] == I_AM_L3GD20) {
        gyro_ready = 1;
    } else if (dt[0] == I_AM_L3GD20_H) {
        gyro_ready = 1;
//        printf("Sensor detected!\r\n");
    } else {
        gyro_ready = 0;
        return;     // gyro chip is NOT on I2C line then terminate this part
    }
    //  Reg.1
    dt[0] = L3GX_CTRL_REG1;
    dt[1] = 0x0f;
    dt[1] |= data_rate << 6;
    dt[1] |= bandwidth << 4;
    _i2c.write(gyro_addr, dt, 2, false);
    //  Reg.3
    dt[0] = L3GX_CTRL_REG3;
    dt[1] = 0x08;
    _i2c.write(gyro_addr, dt, 2, false);
    //  Reg.4
    dt[0] = L3GX_CTRL_REG4;
    switch (fullscale) {
        case L3GX_FS_250DPS:
            fs_factor = 0.00875;
            dt[1] = 0x80;
            break;
        case L3GX_FS_500DPS:
            fs_factor = 0.0175;
            dt[1] = 0x90;
            break;
        case L3GX_FS_2000DPS:
            fs_factor = 0.07;
            dt[1] = 0xa0;
            break;
        default:
            ;
    }
    _i2c.write(gyro_addr, dt, 2, false);
}

void L3GX_GYRO::read_data(float *dt_usr)
{
    char data[6];

    if (gyro_ready == 0) {
        dt_usr[0] = 0;
        dt_usr[1] = 0;
        dt_usr[2] = 0;
        return;
    }
    // X,Y & Z
    // manual said that
    // In order to read multiple bytes, it is necessary to assert the most significant bit
    // of the subaddress field.
    // In other words, SUB(7) must be equal to ‘1’ while SUB(6-0) represents the address
    // of the first register to be read.
    dt[0] = L3GX_OUT_X_L | 0x80;
    _i2c.write(gyro_addr, dt, 1, true);
    _i2c.read(gyro_addr, data, 6, false);
    // data normalization
    dt_usr[0] = float(short(data[1] << 8 | data[0])) * fs_factor;
//    printf("%0.4x;%f\r\n",data[1] << 8 | data[0],dt_usr[0]);
    dt_usr[1] = float(short(data[3] << 8 | data[2])) * fs_factor;
    dt_usr[2] = float(short(data[5] << 8 | data[4])) * fs_factor;
}

int8_t L3GX_GYRO::read_temp()
{
    if (gyro_ready == 1) {
        dt[0] = L3GX_OUT_TEMP;
        _i2c.write(gyro_addr, dt, 1, true);
        _i2c.read(gyro_addr, dt, 1, false);
    } else {
        dt[0] = 99;
    }
    return (int8_t)dt[0];
}

uint8_t L3GX_GYRO::read_id()
{
    dt[0] = L3GX_WHO_AM_I;
    printf("dt[0]: %x\r\n",dt[0]);
    _i2c.write(gyro_addr, dt, 1, true);
    _i2c.read(gyro_addr, dt, 1, false);
    return (uint8_t)dt[0];
}

uint8_t L3GX_GYRO::data_ready()
{
    if (gyro_ready == 1) {
        dt[0] = L3GX_STATUS_REG;
        _i2c.write(gyro_addr, dt, 1, true);
        _i2c.read(gyro_addr, dt, 1, false);
        if (!(dt[0] & 0x01)) {
            return 0;
        }
    }
    return 1;;
}

void L3GX_GYRO::frequency(int hz)
{
    _i2c.frequency(hz);
}

uint8_t L3GX_GYRO::read_reg(uint8_t addr)
{
    if (gyro_ready == 1) {
        dt[0] = addr;
        _i2c.write(gyro_addr, dt, 1, true);
        _i2c.read(gyro_addr, dt, 1, false);
    } else {
        dt[0] = 0xff;
    }
    return (uint8_t)dt[0];
}

void L3GX_GYRO::write_reg(uint8_t addr, uint8_t data)
{
    if (gyro_ready == 1) {
        dt[0] = addr;
        dt[1] = data;
        _i2c.write(gyro_addr, dt, 2, false);
    }
}
