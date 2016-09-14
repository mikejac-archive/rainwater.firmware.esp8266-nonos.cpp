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

#include "accessory_pump.h"
#include <osapi.h>
#include <mem.h>

#define DTXT(...)   os_printf(__VA_ARGS__)
//#define DTXT(...)

/******************************************************************************************************************
 * prototypes
 *
 */

/******************************************************************************************************************
 * public functions
 *
 */

/**
 * 
 * @param name
 * @param serialnumber
 * @param manufacturer
 * @param model
 * @param state
 * @param min
 * @param max
 * @return 
 */
PumpSwitch* ICACHE_FLASH_ATTR NewPumpSwitch(  const char* name, 
                                                                                    const char* serialnumber, 
                                                                                    const char* manufacturer, 
                                                                                    const char* model,
                                                                                    uint8_t     state)
{
    PumpSwitch* acc = (PumpSwitch*) os_zalloc(sizeof(PumpSwitch));
    if(acc == 0) {
        DTXT("NewPumpSwitch(PumpSwitch): mem fail\n");
        return 0;
    }

    acc->Accessory = NewAccessory(name, serialnumber, manufacturer, model, TypeAccProgrammableSwitch);
    if(acc->Accessory == 0) {
        DTXT("NewPumpSwitch(Accessory): mem fail\n");
        goto defer;
    }
    
    acc->StatefulProgrammableSwitch = NewStatefulProgrammableSwitch();
    if(acc->StatefulProgrammableSwitch == 0) {
        DTXT("NewPumpSwitch(StatefulProgrammableSwitch): mem fail\n");
        goto defer;
    }

    acc->StatelessProgrammableSwitch = NewStatelessProgrammableSwitch();
    if(acc->StatelessProgrammableSwitch == 0) {
        DTXT("NewPumpSwitch(StatelessProgrammableSwitch): mem fail\n");
        goto defer;
    }
    
    acc->StatefulProgrammableSwitch->Service->parent                              = acc->Accessory;
    acc->StatelessProgrammableSwitch->Service->parent                             = acc->Accessory;
    acc->StatefulProgrammableSwitch->ProgrammableSwitchEvent->UInt8->parent       = acc->StatefulProgrammableSwitch->Service;
    acc->StatefulProgrammableSwitch->ProgrammableSwitchOutputState->UInt8->parent = acc->StatefulProgrammableSwitch->Service;
    acc->StatelessProgrammableSwitch->ProgrammableSwitchEvent->UInt8->parent      = acc->StatelessProgrammableSwitch->Service;
    
    UInt8SetMinValue(acc->StatefulProgrammableSwitch->ProgrammableSwitchEvent->UInt8, 0);
    UInt8SetMaxValue(acc->StatefulProgrammableSwitch->ProgrammableSwitchEvent->UInt8, 2);
    UInt8SetValue(acc->StatefulProgrammableSwitch->ProgrammableSwitchEvent->UInt8, state);

    UInt8SetMinValue(acc->StatefulProgrammableSwitch->ProgrammableSwitchOutputState->UInt8, 0);
    UInt8SetMaxValue(acc->StatefulProgrammableSwitch->ProgrammableSwitchOutputState->UInt8, 2);
    UInt8SetValue(acc->StatefulProgrammableSwitch->ProgrammableSwitchOutputState->UInt8, state);

    UInt8SetMinValue(acc->StatelessProgrammableSwitch->ProgrammableSwitchEvent->UInt8, 0);
    UInt8SetMaxValue(acc->StatelessProgrammableSwitch->ProgrammableSwitchEvent->UInt8, 1);
    UInt8SetValue(acc->StatelessProgrammableSwitch->ProgrammableSwitchEvent->UInt8, 1);

    AddService(acc->Accessory, acc->StatefulProgrammableSwitch->Service);
    AddService(acc->Accessory, acc->StatelessProgrammableSwitch->Service);
        
    return acc;
    
defer:
    if(acc->StatelessProgrammableSwitch != 0) {
        os_free(acc->StatelessProgrammableSwitch);
    }
    if(acc->StatefulProgrammableSwitch != 0) {
        os_free(acc->StatefulProgrammableSwitch);
    }
    if(acc->Accessory != 0) {
        os_free(acc->Accessory);
    }
    if(acc != 0) {
        os_free(acc);
    }
    return 0;
}
