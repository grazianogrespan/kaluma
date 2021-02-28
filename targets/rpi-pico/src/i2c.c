/* Copyright (c) 2017 Kaluma
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "system.h"
#include "i2c.h"
#include "rpi_pico.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

static struct __i2c_status_s {
  km_i2c_mode_t mode;
} __i2c_status[I2C_NUM];

/**
 * Initialize all I2C when system started
 */
void km_i2c_init() {
  for (int i = 0; i < I2C_NUM; i++)
  {
    __i2c_status[i].mode = KM_I2C_NONE;
  }
}

/**
 * Cleanup all I2C when system cleanup
 */
void km_i2c_cleanup() {
  i2c_deinit(i2c0);
  i2c_deinit(i2c1);
  km_i2c_init();
}

static i2c_inst_t *__get_i2c_no(uint8_t bus) {
  if (bus == 0) {
    return i2c0;
  } else if (bus == 1) {
    return i2c1;
  } else {
    return NULL;
  }
}

int km_i2c_setup_master(uint8_t bus, uint32_t speed) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_NONE)) {
    return KM_I2CPORT_ERROR;
  }
  __i2c_status[bus].mode = KM_I2C_MASTER;
  if (speed > I2C_MAX_CLOCK) {
    speed = I2C_MAX_CLOCK;
  }
  if (bus == 0) {
    gpio_set_function(20, GPIO_FUNC_I2C);
    gpio_set_function(21, GPIO_FUNC_I2C);
    gpio_pull_up(20);
    gpio_pull_up(21);
  } else { // bus 1
    gpio_set_function(18, GPIO_FUNC_I2C);
    gpio_set_function(19, GPIO_FUNC_I2C);
    gpio_pull_up(18);
    gpio_pull_up(19);
  }
  i2c_init(i2c, speed);
  return 0;
}

int km_i2c_setup_slave(uint8_t bus, uint8_t address) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_NONE)) {
    return KM_I2CPORT_ERROR;
  }
  __i2c_status[bus].mode = KM_I2C_SLAVE;
  return 0;
}

int km_i2c_memWrite_master(uint8_t bus, uint8_t address, uint16_t memAddress, uint8_t memAdd16bit, uint8_t *buf, size_t len, uint32_t timeout) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  int ret;
  uint8_t mem_addr[2];
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_MASTER)) {
    return KM_I2CPORT_ERROR;
  }
  if (memAdd16bit == 0) { // 8bit mem address
    mem_addr[0] = (memAddress & 0xFF);
    ret = i2c_write_blocking(i2c, address, mem_addr, 1, true); // true to keep master control of bus
  } else { // 16 bit mem address
    mem_addr[0] = ((memAddress>>8) & 0xFF);
    mem_addr[1] = (memAddress & 0xFF);
    ret = i2c_write_blocking(i2c, address, mem_addr, 2, true); // true to keep master control of bus
  }
  if (ret >= 0) {
    ret = i2c_write_timeout_us(i2c, address, buf, len, false, timeout * 1000);
  }
  if (ret < 0) {
    return -1;
  }
  return ret;
}

int km_i2c_memRead_master(uint8_t bus, uint8_t address, uint16_t memAddress, uint8_t memAdd16bit, uint8_t *buf, size_t len, uint32_t timeout) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  int ret;
  uint8_t mem_addr[2];
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_MASTER)) {
    return KM_I2CPORT_ERROR;
  }
  if (memAdd16bit == 0) { // 8bit mem address
    mem_addr[0] = (memAddress & 0xFF);
    ret = i2c_write_blocking(i2c, address, mem_addr, 1, true); // true to keep master control of bus
  } else { // 16 bit mem address
    mem_addr[0] = ((memAddress>>8) & 0xFF);
    mem_addr[1] = (memAddress & 0xFF);
    ret = i2c_write_blocking(i2c, address, mem_addr, 2, true); // true to keep master control of bus
  }
  if (ret >= 0) {
    ret = i2c_read_timeout_us(i2c, address, buf, len, false, timeout * 1000);
  }
  if (ret < 0) {
    return -1;
  }
  return ret;
}

int km_i2c_write_master(uint8_t bus, uint8_t address, uint8_t *buf, size_t len, uint32_t timeout) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  int ret;
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_MASTER)) {
    return KM_I2CPORT_ERROR;
  }
  ret = i2c_write_timeout_us(i2c, address, buf, len, false, timeout * 1000);
  if (ret < 0) {
    return -1;
  }
  return ret;
}

int km_i2c_write_slave(uint8_t bus, uint8_t *buf, size_t len, uint32_t timeout) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_SLAVE)) {
    return KM_I2CPORT_ERROR;
  }
  return 0;
}

int km_i2c_read_master(uint8_t bus, uint8_t address, uint8_t *buf, size_t len, uint32_t timeout) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  int ret;
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_MASTER)) {
    return KM_I2CPORT_ERROR;
  }
  ret = i2c_read_timeout_us(i2c, address, buf, len, false, timeout * 1000);
  if (ret < 0) {
    return -1;
  }
  return ret;
}

int km_i2c_read_slave(uint8_t bus, uint8_t *buf, size_t len, uint32_t timeout) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  if ((i2c == NULL) || (__i2c_status[bus].mode != KM_I2C_SLAVE)) {
    return KM_I2CPORT_ERROR;
  }
  return 0;
}

int km_i2c_close(uint8_t bus) {
  i2c_inst_t *i2c = __get_i2c_no(bus);
  if ((i2c == NULL) || (__i2c_status[bus].mode == KM_I2C_NONE)) {
    return KM_I2CPORT_ERROR;
  }
  i2c_deinit(i2c);
  __i2c_status[bus].mode = KM_I2C_NONE;
  return 0;
}