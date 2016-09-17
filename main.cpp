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

#include <github.com/mikejac/misc.esp8266-nonos.cpp/espmissingincludes.h>
#include <github.com/mikejac/misc.esp8266-nonos.cpp/uart.h>
#include <github.com/mikejac/date_time.esp8266-nonos.cpp/system_time.h>
#include <github.com/mikejac/timer.esp8266-nonos.cpp/timer.h>
#include <github.com/mikejac/rpcmqtt.esp8266-nonos.cpp/service_device.h>
#include <github.com/mikejac/blinker.esp8266-nonos.cpp/blinker.h>
#include <github.com/mikejac/button.esp8266-nonos.cpp/button.h>
#include <github.com/mikejac/esp-open-rtos.gpio.esp8266-nonos.cpp/gpio/gpio.h>
#include <github.com/mikejac/wifi.esp8266-nonos.cpp/wifi.h>
#include <github.com/mikejac/upgrader.esp8266-nonos.cpp/upgrader.h>
#include <github.com/mikejac/raburton.rboot.esp8266-nonos.cpp/appcode/rboot-api.h>
#include "accessory_pump.h"
#include "user_config.h"
#include "package.h"
#include "wifi.h"

#define DTXT(...)   os_printf(__VA_ARGS__)

// tasks
#define TASK0_ID            0
#define TASK1_ID            1
#define TASK2_ID            2

#define QUEUE_SIZE          2

typedef struct {
    int             pin;
    unsigned long   debounce;
    Timer           debounceTimer;
    int             state;
} WaterLevel;

typedef enum {
    WaterLevelEventNone,
    WaterLevelEventLow,
    WaterLevelEventOk
} WaterLevelEvent;

/******************************************************************************************************************
 * global variables
 *
 */

const char          pkgId[]   = PKG_ID;
const uint32_t      version   = PKG_VERSION;
char                pkg[16];

os_event_t          task0_queue[QUEUE_SIZE];

MqttOptions*        mqttOptions = NULL;
Mqtt*               mqtt        = NULL;
MqttDevice*         device      = NULL;

PumpSwitch*         pump = NULL;

UPGRADER            upgrader;
BLINKER             blueLED;
BLINKER             buttonLED;
BUTTON              button;

WaterLevel          waterLevel;

Timer               periodicInterval = Timer_initializer;

uint8_t             pumpMode;

WIFI_AP             wifi_list[] = {
    { SSID1, PSW1 },
    { SSID2, PSW2 },
    { NULL,  NULL }
};

/******************************************************************************************************************
 * prototypes
 *
 */

/**
 * 
 * @param mode
 */
static void setPumpMode(uint8_t mode);
/**
 * 
 */
static void buttonShortPress(void);
/**
 * 
 * @param w
 * @return 
 */
static WaterLevelEvent getWaterLevel(WaterLevel* w);
/**
 * 
 * @param w
 * @param pin
 * @return 
 */
static int waterLevelInitialize(WaterLevel* w, int pin);
/**
 * 
 */
static void onConnect(void);
/**
 * 
 * @param mqtt
 * @param ptr
 * @param nodename
 * @param actorId
 * @param platformId
 * @param feedId
 * @param payload
 */
static void onCommandCallback(  Mqtt*       mqtt, 
                                void*       ptr,
                                const char* nodename,
                                const char* actorId,
                                const char* platformId,
                                const char* feedId,
                                const char* payload);

/******************************************************************************************************************
 * functions
 *
 */

/**
 * 
 * @param e
 */
void ICACHE_FLASH_ATTR task1(os_event_t* e)
{
    /******************************************************************************************************************
     * 
     * 
     */
    if(pumpMode == PUMP_STATE_AUTO) {
        switch(getWaterLevel(&waterLevel)) {
            case WaterLevelEventNone:
                break;
                
            case WaterLevelEventLow:
                DTXT("AUTO - Off\n");
                gpio_write(GPIO_PUMP, PUMP_OFF);

                PumpSwitchEventSetValue(pump, PUMP_STATE_OFF);
                break;

            case WaterLevelEventOk:
                DTXT("AUTO - On\n");
                gpio_write(GPIO_PUMP, PUMP_ON);

                PumpSwitchEventSetValue(pump, PUMP_STATE_ON);
                break;
        }
    }

    switch(Button_Run(&button)) {
        case Button_None:
            break;
            
        case Button_Short:
            DTXT("Button short press\n");
            buttonShortPress();
            break;
            
        case Button_Long:
            DTXT("Button long press - restart\n");
            system_restart();
            break;
    }
    
    Blinker_Run(&buttonLED);

    /******************************************************************************************************************
     * 
     * 
     */
    if(expired(&periodicInterval)) {
        Info(mqtt, "Running");
        
        countdown(&periodicInterval, PERIODIC_INTERVAL);
    }
    
    /******************************************************************************************************************
     * deal with MQTT
     * 
     */
    int event = ConnectorRun(mqtt);
    if(event == RUN_CONNECTED) {
        onConnect();
    }
    
    /******************************************************************************************************************
     * deal with incoming value updates
     * 
     */
    Device_Message* dm;
    
    switch(DeviceGetEventType(dm = DeviceGetEvent(device))) {
        case FormatString:  
            break;
        case FormatBool:
            DTXT("Bool; aid = %lld, iid = %lld, value = %s\n", Device_GetAid(dm), Device_GetIid(dm), (Device_GetValueBool(dm) == true) ? "True" : "False");
            break;
        case FormatUInt8:   
            DTXT("UInt8; aid = %lld, iid = %lld, value = %d\n", Device_GetAid(dm), Device_GetIid(dm), Device_GetValueUInt8(dm));
            if(dm->acc == pump->Accessory) {
                DTXT("pump\n");
                
                if(Device_GetIid(dm) == PumpSwitchOutputStateIid) {
                    setPumpMode(Device_GetValueUInt8(dm));
                }
            }
            break;
        case FormatInt8:    
            break;
        case FormatUInt16:  
            break;
        case FormatInt16:   
            break;
        case FormatUInt32:  
            break;
        case FormatInt32:   
            break;
        case FormatUInt64:  
            break;
        case FormatFloat:   
            break;
        case FormatNone:
            break;
    }
    
    DeviceDeleteEvent(dm);

    /******************************************************************************************************************
     * show we're alive
     * 
     */
    if(WIFI_IsConnected()) {
        if(IsConnected(mqtt)) {
            Blinker_Set(&blueLED, 200, 3000);
        } else {
            Blinker_Set(&blueLED, 1000, 1000);
        }
    } else {
        Blinker_Set(&blueLED, 200, 200);
    }
    
    Blinker_Run(&blueLED);

    /******************************************************************************************************************
     * firmware upgrader
     * 
     */
    if(Upgrader_NewPkg(&upgrader)) {
        if(Close(mqtt)) {
            Upgrader_Run(&upgrader);
        }
    }
    
    system_os_post(TASK1_ID, 0, 0);
}
/**
 * 
 * @param mode
 */
void ICACHE_FLASH_ATTR setPumpMode(uint8_t mode)
{
    switch(mode) {
        case PUMP_STATE_OFF:
            DTXT("setPumpMode(): OFF\n");
            pumpMode = PUMP_STATE_OFF;
            gpio_write(GPIO_PUMP, PUMP_OFF);
            
            PumpSwitchOutputStateSetValue(pump, PUMP_STATE_OFF);
            PumpSwitchEventSetValue(pump, PUMP_STATE_OFF);
            Blinker_Set(&buttonLED, 200, 3000);
            break;
            
        case PUMP_STATE_ON:
            DTXT("setPumpMode(): ON\n");
            pumpMode = PUMP_STATE_ON;
            gpio_write(GPIO_PUMP, PUMP_ON);

            PumpSwitchOutputStateSetValue(pump, PUMP_STATE_ON);
            PumpSwitchEventSetValue(pump, PUMP_STATE_ON);
            Blinker_Set(&buttonLED, 3000, 200);
            break;
            
        case PUMP_STATE_AUTO:
            DTXT("setPumpMode(): AUTO\n");
            pumpMode = PUMP_STATE_AUTO;

            PumpSwitchOutputStateSetValue(pump, PUMP_STATE_AUTO);
            Blinker_Set(&buttonLED, 1000, 1000);
            break;
    }
}
/**
 * 
 */
void ICACHE_FLASH_ATTR buttonShortPress(void)
{
    // cycle through the pump modes
    switch(pumpMode) {
        case PUMP_STATE_OFF:
            DTXT("buttonShortPress(): OFF -> ON\n");
            setPumpMode(PUMP_STATE_ON);
            break;
            
        case PUMP_STATE_ON:
            DTXT("buttonShortPress(): ON -> AUTO\n");
            setPumpMode(PUMP_STATE_AUTO);
            break;
            
        case PUMP_STATE_AUTO:
            DTXT("buttonShortPress(): AUTO -> OFF\n");
            setPumpMode(PUMP_STATE_OFF);
            break;
    }
}
/**
 * 
 * @param w
 * @return 
 */
WaterLevelEvent ICACHE_FLASH_ATTR getWaterLevel(WaterLevel* w)
{
    bool bb = gpio_read(w->pin);    // true => switch is open => low water level

    if(w->state == -1) {            // first run after startup
        if(bb == true) {
            w->state = 1;
            
            // start debounce timer
            countdown_ms(&w->debounceTimer, w->debounce);
            
            DTXT("getWaterLevel(state = -1): WaterLevelEventLow\n");
            return WaterLevelEventLow;
        } else {
            w->state = 0;

            DTXT("getWaterLevel(state = -1): WaterLevelEventOk\n");
            return WaterLevelEventOk;
        }
    } else if(w->state == 0) {
        if(expired(&w->debounceTimer)) {
            if(bb == true) {
                // start debounce timer
                countdown_ms(&w->debounceTimer, w->debounce);
                w->state = 1;

                DTXT("getWaterLevel(state = 0): WaterLevelEventLow\n");
                return WaterLevelEventLow;
            }
        }
    } else if(w->state == 1) {
        if(expired(&w->debounceTimer)) {
            if(bb == false) {
                // start debounce timer
                countdown_ms(&w->debounceTimer, w->debounce);
                w->state = 0;
                
                DTXT("getWaterLevel(state = 1): WaterLevelEventOk\n");
                return WaterLevelEventOk;
            }
        }
    } 
    
    return WaterLevelEventNone;
}
/**
 * 
 * @param w
 * @param pin
 * @return 
 */
int ICACHE_FLASH_ATTR waterLevelInitialize(WaterLevel* w, int pin)
{
    w->pin        = pin;
    w->debounce   = 60 * 1000;
    w->state      = -1;
    
    // disable debounce timer
    countdown_ms(&w->debounceTimer, 0);
    
    gpio_enable(pin, GPIO_INPUT_PULLUP);
    
    return 0;
}
/**
 * 
 */
void ICACHE_FLASH_ATTR onConnect(void)
{
    DTXT("onConnect():\n");

    // firmware upgrade service
    Upgrader_Subscribe_Package(&upgrader);
    Upgrader_Publish_Package(&upgrader);
}
/**
 * 
 * @param mqtt
 * @param ptr
 * @param nodename
 * @param actorId
 * @param platformId
 * @param feedId
 * @param payload
 */
void ICACHE_FLASH_ATTR onCommandCallback(Mqtt*          mqtt, 
                                         void*          ptr,
                                         const char*    nodename,
                                         const char*    actorId,
                                         const char*    platformId,
                                         const char*    feedId,
                                         const char*    payload)
{
    if(Upgrader_Check(&upgrader, nodename, actorId, platformId, feedId, payload)) {
        
    } else {
        DTXT("onCommandCallback(): command message\n");
        DTXT("onCommandCallback(): nodename          = %s\n", nodename);
        DTXT("onCommandCallback(): actor_id          = %s\n", actorId);
        DTXT("onCommandCallback(): platform_id       = %s\n", platformId);
        DTXT("onCommandCallback(): feed_id           = %s\n", feedId);
        DTXT("onCommandCallback(): payload           = %s\n", payload);
    }
}
/**
 * 
 */
void ICACHE_FLASH_ATTR main_init_done(void)
{
    DTXT("main_init_done(): begin\n");

    os_sprintf(pkg, "%s %lu", pkgId, version);
    
    Container* cont = NewContainer(WIFI_GetMAC(), "Regnvand", "001", "github.com/mikejac", pkg, 6 * 1024);

    pump = NewPumpSwitch("Pumpe", "001-01", "github.com/mikejac", "ESP8266", PUMP_STATE_AUTO);
    
    AddAccessory(cont, pump->Accessory);

    WIFI_Run();
    
    mqttOptions = NewMqttOptions();
    if(mqttOptions != 0) {
        MqttOptions_SetServer(mqttOptions,          MQTT_BROKER_IP);
        MqttOptions_SetPort(mqttOptions,            MQTT_BROKER_PORT);
        MqttOptions_SetClientId(mqttOptions,        WIFI_GetMAC());
        MqttOptions_SetKeepalive(mqttOptions,       MQTT_KEEPALIVE);
        MqttOptions_SetRootTopic(mqttOptions,       FABRIC_ROOT_TOPIC);
        MqttOptions_SetNodename(mqttOptions,        WIFI_GetMAC());
        MqttOptions_SetActorPlatformId(mqttOptions, MY_PLATFORM_ID);
        MqttOptions_SetClassType(mqttOptions,       ClassTypeDeviceSvc);
        MqttOptions_SetBufferSize(mqttOptions,      6 * 1024);
        
        mqtt = Connector(mqttOptions);
        if(mqtt != 0) {
            InstallCallbacks(   mqtt,
                                NULL,
                                onCommandCallback);

            EnableChronos(mqtt, NODENAME_CHRONOS);

            device = NewDevice(mqtt);

            // 'device' takes ownership of container when set
            SetAccessories(device, cont);

            Upgrader_Initialize(&upgrader, mqtt, NODENAME_UPGRADER, pkgId, &version);
        }
    }
            
    Blinker_Initialize(&blueLED, GPIO_BLUE);
    Blinker_Set(&blueLED, 200, 200);
    Blinker_Enable_on(&blueLED);

    Blinker_Initialize(&buttonLED, GPIO_BUTTON_LED);
    Blinker_Set(&buttonLED, 200, 200);
    Blinker_Enable_on(&buttonLED);

    Button_Initialize(&button, GPIO_BUTTON, 1);
    Button_SetShortPress(&button, 300);
    Button_SetLongPress(&button, 5000);

    waterLevelInitialize(&waterLevel, GPIO_WATER_LEVEL);
    
    gpio_enable(GPIO_PUMP, GPIO_OUTPUT);

    setPumpMode(PUMP_STATE_AUTO);
    
    countdown(&periodicInterval, PERIODIC_INTERVAL);
    
    // create the so-called task
    system_os_task(task1, TASK1_ID, task0_queue, QUEUE_SIZE);

    system_os_post(TASK1_ID, 0, 0);

    DTXT("main_init_done(): end\n");
}
/*
 * 
 */
extern "C" void ICACHE_FLASH_ATTR user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(3000000);
    
    DTXT("\n\n");
    DTXT("user_init(): begin\n");
    
    os_printf("Current ROM ....: %d\n",  rboot_get_current_rom());
    os_printf("Package ID .....: %s\n",  pkgId);
    os_printf("Package version : %lu\n", version);
    
    esp_start_system_time();                                                    // start our 1 second clock
    
    WIFI_InitializeEx(wifi_list);
    
    system_init_done_cb(main_init_done);
    
    DTXT("user_init(): end\n");
}
