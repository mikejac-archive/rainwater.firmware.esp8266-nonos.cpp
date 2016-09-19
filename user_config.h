/* 
 * The MIT License (MIT)
 * 
 * ESP8266 Non-OS Firmware
 * Copyright (c) 2015 Michael Jacobsen (github.com/mikejac)
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

#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define PERIODIC_INTERVAL            60

/******************************************************************************************************************
 * MQTT
 *
 */
#define MQTT_BROKER_IP          "192.168.1.82"
#define MQTT_BROKER_PORT        1883
#define MQTT_KEEPALIVE          60
#define MQTT_CLEAN_SESSION      1

#define MQTT_QOS                0

#define FABRIC_ROOT_TOPIC       "fabric"

/******************************************************************************************************************
 * fabric
 *
 */
#define MY_PLATFORM_ID          "rainwater"

// external nodenames
#define NODENAME_CHRONOS        "134806d8-f9e6-43dd-9f0c-bbb1634dc956"
#define NODENAME_UPGRADER       "f330b8cb-e559-4590-bae0-d079b1b5e7e4"

/******************************************************************************************************************
 * pump
 *
 */
#define PUMP_STATE_OFF        0
#define PUMP_STATE_ON         1
#define PUMP_STATE_AUTO       2

/******************************************************************************************************************
 * GPIO
 *
 */

// blue LED on ESP-12(E)
#define GPIO_BLUE               2

// relays
#define GPIO_REL1               14

// button and LED in button
#define GPIO_BUTTON             4
#define GPIO_BUTTON_LED         12

// water level switch
#define GPIO_WATER_LEVEL        5

#define GPIO_PUMP               GPIO_REL1

/******************************************************************************************************************
 * various
 *
 */
#define PUMP_ON                  false
#define PUMP_OFF                 true

#endif
