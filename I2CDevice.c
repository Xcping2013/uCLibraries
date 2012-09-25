/**
 *  @file       I2CDevice.h
 *  @brief      Main I2C device functions
 *  @details    Abstracts bit and byte I2C R/W functions
 *  @author     Jeff Rowberg
 *  @author     Luis Maduro (Port from C++ to C and adapted to PIC18)
 *  @version    1.0
 *  @date       June 2012
 *  @copyright  MIT License.
 *
 * I2Cdev device library code is placed under the MIT license
 * Copyright (c) 2011 Jeff Rowberg
 * Copyright (c) 2012 Luis Maduro
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <p18cxxx.h>
#include "I2CDevice.h"

/**
 * Sends a start condition to the I2C bus.
 * @warning Hardware specific!
 */
void I2CStart(void)
{
    SSP1CON2bits.SEN = 1;
    while (SSP1CON2bits.SEN);
}

/**
 * Sends a restart condition to the I2C bus.
 * @warning Hardware specific!
 */
void I2CRestart(void)
{
    SSP1CON2bits.RSEN = 1;
    while (SSP1CON2bits.RSEN);
}

/**
 * Sends a stop condition to the I2C bus.
 * @warning Hardware specific!
 */
void I2CStop(void)
{
    SSP1CON2bits.PEN = 1;
    while (SSP1CON2bits.PEN);
}

/**
 * Sends an ack to the slave.
 * @warning Hardware specific!
 */
void I2CAck(void)
{
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
    while (SSP1CON2bits.ACKEN);
}

/**
 * Sends a not ack to the slave.
 * @warning Hardware specific!
 */
void I2CNotAck(void)
{
    SSP1CON2bits.ACKDT = 1;
    SSP1CON2bits.ACKEN = 1;
    while (SSP1CON2bits.ACKEN);
}

/**
 * Sends a byte througth the I2C bus.
 * @param data_out Data byte to be sent.
 * @return 0 if all went well !=0 if not.
 */
unsigned char I2CWrite(unsigned char data_out)
{
    SSP1BUF = data_out;

    if (SSP1CON1bits.WCOL)
        return ( -1);

    else
    {
        if (((SSP1CON1 & 0x0F) != 0x08) && ((SSP1CON1 & 0x0F) != 0x0B))
        {
            SSP1CON1bits.CKP = 1;

            while (!PIR1bits.SSP1IF);

            if ((!SSP1STATbits.R_W) && (!SSP1STATbits.BF))
                return (-2);
            else
                return (0);

        }
        else if (((SSP1CON1 & 0x0F) == 0x08) || ((SSP1CON1 & 0x0F) == 0x0B))
        {
            while (SSP1STATbits.BF);

            while ((SSP1CON2 & 0x1F) | (SSP1STATbits.R_W));

            if (SSP1CON2bits.ACKSTAT)
                return ( -2);
            else
                return ( 0);
        }
    }
}

/**
 * Reads a byte from the I2C bus.
 * @return The bytes retrieved.
 */
unsigned char I2CRead(void)
{
    if (((SSP1CON1 & 0x0F) == 0x08) || ((SSP1CON1 & 0x0F) == 0x0B))
        SSP1CON2bits.RCEN = 1;
    while (!SSP1STATbits.BF);
    return ( SSP1BUF);
}

/**
 * Read multiple bytes from a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address First register address to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 */
void I2CDeviceReadBytes(unsigned char deviceAddress,
                        unsigned char address,
                        unsigned char length,
                        unsigned char *data)
{
    unsigned char i = 0;

    I2CStart();
    I2CWrite(deviceAddress & 0xFE);
    I2CWrite(address);
    I2CRestart();

    I2CWrite(deviceAddress | 0x01);

    for (i = 0; i < length; i++)
    {
        data[i] = I2CRead();

        if (i == (length - 1))
        {
            I2CNotAck();
        }
        else
        {
            I2CAck();
        }
    }

    I2CStop();
}

/**
 * Write multiple bytes to a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 */
void I2CDeviceWriteBytes(unsigned char deviceAddress,
                         unsigned char address,
                         unsigned char length,
                         unsigned char *data)
{
    unsigned char i;

    I2CStart();
    I2CWrite(deviceAddress & 0xFE);
    I2CWrite(address);

    for (i = 0; i < length; i++)
    {
        I2CWrite(data[i]);
    }
    I2CStop();
}

/**
 * Read a single bit from a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address Register address to read from
 * @param _bit Bit in register position to read (0-7)
 * @return Single bit value
 */
unsigned char I2CDeviceReadBit(unsigned char deviceAddress,
                               unsigned char address,
                               unsigned char _bit)
{
    return (I2CDeviceReadByte(deviceAddress, address) & (1 << _bit));
}

/**
 * Read multiple bits from a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address Register address to read from
 * @param bitStart First bit position to read (0-7)
 * @param length Number of bits to read (not more than 8)
 * @return Right-aligned value (i.e. '101' read from any bitStart position will
 *                              equal 0x05)
 */
unsigned char I2CDeviceReadBits(unsigned char deviceAddress,
                                unsigned char address,
                                unsigned char bitStart,
                                unsigned char length)
{
    // 01101001 read byte
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    //    010   masked
    //   -> 010 shifted
    unsigned char i;
    unsigned char b;
    unsigned char r = 0;

    b = I2CDeviceReadByte(deviceAddress, address);

    for (i = bitStart; i > bitStart - length; i--)
    {
        r |= (b & (1 << i));
    }
    r >>= (bitStart - length + 1);

    return r;
}

/**
 * Read single byte from a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address Register address to read from
 * @return Byte value read from device
 */
unsigned char I2CDeviceReadByte(unsigned char deviceAddress,
                                unsigned char address)
{
    unsigned char b = 0;
    I2CDeviceReadBytes(deviceAddress, address, 1, &b);
    return b;
}

/**
 * Write a single bit to a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address Register address to write to
 * @param _bit Bit position to write (0-7)
 * @param value New bit value to write
 */
void I2CDeviceWriteBit(unsigned char deviceAddress,
                       unsigned char address,
                       unsigned char _bit,
                       unsigned char value)
{
    unsigned char b;

    b = I2CDeviceReadByte(deviceAddress, address);

    b = value ? (b | (1 << _bit)) : (b & ~(1 << _bit));

    I2CDeviceWriteByte(deviceAddress, address, b);
}

/**
 * Write multiple bits to a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address Register address to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param value Right-aligned value to write
 */
void I2CDeviceWriteBits(unsigned char deviceAddress,
                        unsigned char address,
                        unsigned char bitStart,
                        unsigned char length,
                        unsigned char value)
{
    //      010 value to write
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    // 01000000 shift left (8 - length)    ]
    // 00001000 shift right (7 - bitStart) ] ---    two shifts ensure all
    //                                              non-important bits are 0
    // 11100011 mask byte
    // 10101111 original value (sample)
    // 10100011 original & mask
    // 10101011 masked | value
    unsigned char b;
    unsigned char mask;

    b = I2CDeviceReadByte(deviceAddress, address);

    mask = (0xFF << (8 - length)) | (0xFF >> (bitStart + length - 1));

    value <<= (8 - length);

    value >>= (7 - bitStart);

    b &= mask;

    b |= value;

    I2CDeviceWriteByte(deviceAddress, address, b);
}

/**
 * Write single byte to a device register.
 * @param deviceAddress Address of the device in I2C bus
 * @param address Register address to write to
 * @param value New byte value write
 */
void I2CDeviceWriteByte(unsigned char deviceAddress,
                        unsigned char address,
                        unsigned char value)
{
    I2CDeviceWriteBytes(deviceAddress, address, 1, &value);
}