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

#ifndef ACCESSORY_PUMP_H
#define	ACCESSORY_PUMP_H

#include <github.com/mikejac/rpcmqtt.esp8266-nonos.cpp/svc_accessory.h>

#ifdef	__cplusplus
extern "C" {
#endif

/******************************************************************************************************************
 * 
 *
 */
    
#define PumpSwitchOutputStateSetValue(a, v)     UInt8SetValue((a)->StatefulProgrammableSwitch->ProgrammableSwitchOutputState->UInt8, (v))
#define PumpSwitchOutputStateGetValue(a)        ((a)->StatefulProgrammableSwitch->ProgrammableSwitchOutputState->UInt8->Value->UInt8)

#define PumpSwitchEventSetValue(a, v)           UInt8SetValue((a)->StatelessProgrammableSwitch->ProgrammableSwitchEvent->UInt8, (v))
#define PumpSwitchEventGetValue(a)              ((a)->StatelessProgrammableSwitch->ProgrammableSwitchEvent->UInt8->Value->UInt8)
    
#define PumpSwitchOutputStateIid                9

typedef struct {
    Accessory*                      Accessory;
    
    StatefulProgrammableSwitch*     StatefulProgrammableSwitch;
    StatelessProgrammableSwitch*    StatelessProgrammableSwitch;
} PumpSwitch;
/**
 * 
 * @param name
 * @param serialnumber
 * @param manufacturer
 * @param model
 * @param state
 * @return 
 */
PumpSwitch* NewPumpSwitch(const char* name, 
                                                                const char* serialnumber, 
                                                                const char* manufacturer, 
                                                                const char* model,
                                                                uint8_t     state);

/******************************************************************************************************************
 * prototypes
 *
 */

#ifdef	__cplusplus
}
#endif

#endif	/* ACCESSORY_PUMP_H */

