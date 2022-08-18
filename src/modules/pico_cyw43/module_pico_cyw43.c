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

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "err.h"
#include "jerryscript.h"
#include "jerryxx.h"
#include "pico/cyw43_arch.h"
#include "pico_cyw43_magic_strings.h"

#define MAX_GPIO_NUM 2

JERRYXX_FUN(pico_cyw43_ctor_fn) {
  cyw43_arch_deinit();
  return jerry_create_undefined();
}

/**
 * PICO_CYW43.prototype.close() function
 */
JERRYXX_FUN(pico_cyw43_close_fn) {
  int ret = cyw43_arch_init();
  if (ret) {
    return jerry_create_error_from_value(create_system_error(ret), true);
  }
  return jerry_create_undefined();
}

static int __check_gpio(uint32_t pin) {
  if (pin > MAX_GPIO_NUM) {
    return -1;
  }
  return 0;
}

JERRYXX_FUN(pico_cyw43_get_gpio) {
  JERRYXX_CHECK_ARG_NUMBER(0, "gpio");
  uint32_t gpio = JERRYXX_GET_ARG_NUMBER(0);
  if (__check_gpio(gpio) < 0) {
    return jerry_create_error(JERRY_ERROR_TYPE,
                              (const jerry_char_t *)"GPIO pin is not exist");
  }
  bool ret = cyw43_arch_gpio_get(gpio);
  return jerry_create_boolean(ret);
}

JERRYXX_FUN(pico_cyw43_put_gpio) {
  JERRYXX_CHECK_ARG_NUMBER(0, "gpio");
  JERRYXX_CHECK_ARG_BOOLEAN(1, "value");
  uint32_t gpio = JERRYXX_GET_ARG_NUMBER(0);
  bool value = JERRYXX_GET_ARG_BOOLEAN(1);
  if (__check_gpio(gpio) < 0) {
    return jerry_create_error(JERRY_ERROR_TYPE,
                              (const jerry_char_t *)"GPIO pin is not exist");
  }
  cyw43_arch_gpio_put(gpio, value);
  return jerry_create_undefined();
}

static jerry_value_t scan_result(void *env, const cyw43_ev_scan_result_t *result) {

    jerry_value_t scanResult = &env;

    if (result) {
            jerryxx_set_property_string(scanResult, "ssid", result->ssid);
            jerryxx_set_property_string(scanResult, "rssi", result->rssi);
            jerryxx_set_property_string(scanResult, "channel", result->channel);
            jerryxx_set_property_string(scanResult, "auth_mode", result->auth_mode);
    }

    printf("PicoW scan completed\n");
    //printf(scanResult);
    return scanResult;
}


JERRYXX_FUN(pico_cyw43_scan_fn) {
  int ret = init_cyw43();

  jerry_value_t scanResult = jerry_create_object();

  if(ret == 0){
    cyw43_arch_enable_sta_mode();

    cyw43_wifi_scan_options_t scan_options = {0};

    int exit = 10;

    while(exit > 0) {
        int err = cyw43_wifi_scan(&cyw43_state, &scan_options, &scanResult, scan_result);
        if (err == 0) {
            printf("\nPerforming wifi scan\n");
            return jerry_create_undefined();
        } else {
            printf("Failed to start scan: %d\n", err);
            return jerry_create_undefined();
        }

        sleep_ms(1000);
        exit = exit - 1;
    }
  }

  deinit_cyw43();
  return scanResult;

}

int init_cyw43(){
  if(!cyw43_is_initialized(&cyw43_state)){
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return 1;
    }
    return 0;
  }
  return 0;
}

void deinit_cyw43(){
  if(cyw43_is_initialized(&cyw43_state)){
    cyw43_arch_deinit();
  }
}

int disconnect(int itf){
  return cyw43_wifi_leave(&cyw43_state, itf);
}

int connect(char *wifi_ssid, char *wifi_password, char* cyw43_auth, uint32_t timeout) {

  uint32_t auth;

  if(strcmp(cyw43_auth,"CYW43_AUTH_OPEN") == 0){
    auth = CYW43_AUTH_OPEN;
  }else if(strcmp(cyw43_auth,"CYW43_AUTH_WPA_TKIP_PSK") == 0){
    auth = CYW43_AUTH_WPA_TKIP_PSK;
  }else if(strcmp(cyw43_auth,"CYW43_AUTH_WPA2_AES_PSK") == 0){
    auth = CYW43_AUTH_WPA2_AES_PSK;
  }else if(strcmp(cyw43_auth,"CYW43_AUTH_WPA2_MIXED_PSK") == 0){
    auth = CYW43_AUTH_WPA2_MIXED_PSK;
  }else{
    wifi_password = NULL;
    auth = CYW43_AUTH_OPEN;
  }

  int ret = init_cyw43();

  if(ret == 0){

    cyw43_arch_enable_sta_mode();

    printf("Trying to connect...\n");

    /*
    printf(wifi_ssid);
    printf(wifi_password);
    printf("%" PRIu32 "\n",auth);
    printf("%" PRIu32 "\n",timeout);
    */
    return cyw43_arch_wifi_connect_timeout_ms(wifi_ssid, wifi_password, auth, timeout);

  }else{
    return ret;
  }
}

JERRYXX_FUN(pico_cyw43_disconnect_fn) {
  int ret = disconnect(CYW43_ITF_STA);
  if (ret != 0) {
    return jerry_create_error_from_value(create_system_error(ret), true);
  }
  printf("PicoW disconnected\n");

  return jerry_create_number(ret);
}

JERRYXX_FUN(pico_cyw43_link_status_fn) {
  return jerry_create_number(cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA));
}

JERRYXX_FUN(pico_cyw43_connect_fn) {

  JERRYXX_CHECK_ARG_STRING(0, "wifi_ssid");
  JERRYXX_CHECK_ARG_STRING(1, "wifi_password");
  JERRYXX_CHECK_ARG_STRING(2, "cyw43_auth");
  JERRYXX_CHECK_ARG_NUMBER(3, "timeout");

  JERRYXX_GET_ARG_STRING_AS_CHAR(0, wifi_ssid)
  JERRYXX_GET_ARG_STRING_AS_CHAR(1, wifi_password)
  JERRYXX_GET_ARG_STRING_AS_CHAR(2, cyw43_auth)
  uint32_t timeout = (uint32_t)JERRYXX_GET_ARG_NUMBER(3);

  int ret = connect(wifi_ssid, wifi_password, cyw43_auth, timeout);

  if (ret != 0) {
    return jerry_create_error_from_value(create_system_error(ret), true);
  }

  printf("PicoW connected\n");

  return jerry_create_number(ret);
}

jerry_value_t module_pico_cyw43_init() {
  /* PICO_CYW43 class */
  jerry_value_t pico_cyw43_ctor =
      jerry_create_external_function(pico_cyw43_ctor_fn);
  jerry_value_t prototype = jerry_create_object();
  jerryxx_set_property(pico_cyw43_ctor, "prototype", prototype);
  jerryxx_set_property_function(prototype, MSTR_PICO_CYW43_CLOSE,
                                pico_cyw43_close_fn);
  jerryxx_set_property_function(prototype, MSTR_PICO_CYW43_GETGPIO,
                                pico_cyw43_get_gpio);
  jerryxx_set_property_function(prototype, MSTR_PICO_CYW43_PUTGPIO,
                                pico_cyw43_put_gpio);
  //jerryxx_set_property_function(prototype, MSTR_PICO_CYW43_SCAN,
  //                              pico_cyw43_scan_fn);
  jerryxx_set_property_function(prototype, MSTR_PICO_CYW43_CONNECT,
                                pico_cyw43_connect_fn);
  jerryxx_set_property_function(prototype, MSTR_PICO_CYW43_DISCONNECT,
                                pico_cyw43_disconnect_fn);
  jerryxx_set_property_function(prototype, MSTR_PICO_CYW43_LINK_STATUS,
                                pico_cyw43_link_status_fn);
  jerry_release_value(prototype);

  /* pico_cyw43 module exports */
  jerry_value_t exports = jerry_create_object();
  jerryxx_set_property(exports, MSTR_PICO_CYW43_PICO_CYW43, pico_cyw43_ctor);
  jerry_release_value(pico_cyw43_ctor);

  return exports;
}
