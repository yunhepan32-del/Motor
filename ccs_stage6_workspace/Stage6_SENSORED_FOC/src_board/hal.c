//#############################################################################
// $Copyright:
// Copyright (C) 2017-2025 Texas Instruments Incorporated - http://www.ti.com/
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################


//! \file   solutions/universal_motorcontrol_lab/f28003x/drivers/hal_all.c
//! \brief  Contains the various functions related to the HAL object
//!
//

//
// the includes
//
#include "user.h"


//
// drivers
//

// modules

// platforms
#include "hal.h"
#include "hal_obj.h"

// libraries
#include "datalogIF.h"

#ifdef _FLASH
#pragma CODE_SECTION(Flash_initModule, ".TI.ramfunc");
#endif

// **************************************************************************
// the defines


// **************************************************************************
// the globals
HAL_Handle    halHandle;      //!< the handle for the hardware abstraction layer
HAL_Obj       hal;            //!< the hardware abstraction layer object
#pragma DATA_SECTION(halHandle, "hal_data");
#pragma DATA_SECTION(hal, "hal_data");

// **************************************************************************
// the functions

void HAL_disableGlobalInts(HAL_Handle handle)
{
    // disable global interrupts
    Interrupt_disableMaster();

    return;
} // end of HAL_disableGlobalInts() function

void HAL_disableWdog(HAL_Handle halHandle)
{
    // disable watchdog
    SysCtl_disableWatchdog();

    return;
} // end of HAL_disableWdog() function

void HAL_enableCtrlInts(HAL_Handle handle)
{
    gWildfire.blockedConfig++;
} // end of HAL_enableCtrlInts() function

// No HAL_enableADCIntsToTriggerCLA() in this device

void HAL_enableDebugInt(HAL_Handle handle)
{
    gWildfire.blockedConfig++;
} // end of HAL_enableDebugInt() function

void HAL_enableGlobalInts(HAL_Handle handle)
{
    gWildfire.blockedConfig++;
} // end of HAL_enableGlobalInts() function


HAL_Handle HAL_init(void *pMemory,const size_t numBytes)
{
    HAL_Handle handle;
    HAL_Obj *obj;

    if(numBytes < sizeof(HAL_Obj))
    {
        return((HAL_Handle)NULL);
    }

    // assign the handle
    handle = (HAL_Handle)pMemory;

    // assign the object
    obj = (HAL_Obj *)handle;

    // Two ADC modules in this device
    // initialize the ADC handles
    obj->adcHandle[0] = ADCA_BASE;
    obj->adcHandle[1] = ADCB_BASE;
    obj->adcHandle[2] = ADCC_BASE;

    // initialize the ADC results
    obj->adcResult[0] = ADCARESULT_BASE;
    obj->adcResult[1] = ADCBRESULT_BASE;
    obj->adcResult[2] = ADCCRESULT_BASE;

    // No DAC modules in this device
    // No CLA module in this device

    // initialize SCI handle
    obj->sciHandle = SCIA_BASE;             //!< the SCIA handle

    // initialize CAN handle
    obj->canHandle = CANA_BASE;             //!< the CANA handle

    // initialize DMA handle
    obj->dmaHandle = DMA_BASE;              //!< the DMA handle

#if defined(DATALOGF2_EN)
    // initialize DMA channel handle
    obj->dmaChHandle[0] = DMA_DATALOG1_BASE;     //!< the DMA Channel handle
    obj->dmaChHandle[1] = DMA_DATALOG2_BASE;     //!< the DMA Channel handle
#elif defined(DATALOGF4_EN) || defined(DATALOGI4_EN)
    obj->dmaChHandle[0] = DMA_DATALOG1_BASE;     //!< the DMA Channel handle
    obj->dmaChHandle[1] = DMA_DATALOG2_BASE;     //!< the DMA Channel handle
    obj->dmaChHandle[2] = DMA_DATALOG3_BASE;     //!< the DMA Channel handle
    obj->dmaChHandle[3] = DMA_DATALOG4_BASE;     //!< the DMA Channel handle
#endif  // DATALOGF2_EN | DATALOGF4_EN

    // initialize timer handles
    obj->timerHandle[0] = CPUTIMER0_BASE;
    obj->timerHandle[1] = CPUTIMER1_BASE;
    obj->timerHandle[2] = CPUTIMER2_BASE;

#if defined(EPWMDAC_MODE)
#if defined(HVMTRPFC_REV1P1)
    // initialize pwmdac handles
    obj->pwmDACHandle[0] = EPWMDAC1_BASE;
    obj->pwmDACHandle[1] = EPWMDAC2_BASE;
    obj->pwmDACHandle[2] = EPWMDAC3_BASE;
    obj->pwmDACHandle[3] = EPWMDAC4_BASE;
    // HVMTRPFC_REV1P1
#else
#error EPWMDAC is not supported on this kit!
#endif  // !HVMTRPFC_REV1P1
#endif  // EPWMDAC_MODE


    return(handle);
} // end of HAL_init() function

HAL_MTR_Handle HAL_MTR1_init(void *pMemory, const size_t numBytes)
{
    HAL_MTR_Handle handle;
    HAL_MTR_Obj *obj;

    if(numBytes < sizeof(HAL_MTR_Obj))
    {
        return((HAL_MTR_Handle)NULL);
    }

    // assign the handle
    handle = (HAL_MTR_Handle)pMemory;

    // assign the object
    obj = (HAL_MTR_Obj *)handle;

    // initialize PWM handles for Motor 1
    obj->pwmHandle[0] = MTR1_PWM_U_BASE;        //!< the PWM handle
    obj->pwmHandle[1] = MTR1_PWM_V_BASE;        //!< the PWM handle
    obj->pwmHandle[2] = MTR1_PWM_W_BASE;        //!< the PWM handle

    // initialize CMPSS handle
#if defined(MOTOR1_ISBLDC)
    obj->cmpssHandle[0] = MTR1_CMPSS_IDC_BASE;  //!< the CMPSS handle
#elif defined(MOTOR1_DCLINKSS)
    obj->cmpssHandle[0] = MTR1_CMPSS_IDC_BASE;  //!< the CMPSS handle
#else   // !(MOTOR1_ISBLDC || MOTOR1_DCLINKSS)
    obj->cmpssHandle[0] = MTR1_CMPSS_U_BASE;    //!< the CMPSS handle
    obj->cmpssHandle[1] = MTR1_CMPSS_V_BASE;    //!< the CMPSS handle
    obj->cmpssHandle[2] = MTR1_CMPSS_W_BASE;    //!< the CMPSS handle
#endif  // !(MOTOR1_ISBLDC || MOTOR1_DCLINKSS)

#if defined(MOTOR1_HALL) && defined(CMD_CAP_EN)
#error HALL and CMD_CAP can't be enabled at the same time
#elif defined(MOTOR1_HALL)
    // initialize CAP handles for Motor 1
    obj->capHandle[0] = MTR1_CAP_U_BASE;        //!< the CAP handle
    obj->capHandle[1] = MTR1_CAP_V_BASE;        //!< the CAP handle
    obj->capHandle[2] = MTR1_CAP_W_BASE;        //!< the CAP handle
#elif defined(CMD_CAP_EN)
    obj->capHandle = MTR1_CAP_FREQ_BASE;        //!< the CAP handle
#endif // MOTOR1_HALL || CMD_CAP_EN

    // No PGA modules in this device

    // Assign gateEnableGPIO
#if defined(DRV8329AEVM_REVA)
    obj->gateEnableGPIO = MTR1_GATE_EN_GPIO;
    obj->gateSleepGPIO = MTR1_GATE_nSLEEP_GPIO;
    // DRV8329AEVM_REVA
#elif defined(BSXL8323RS_REVA)
    // initialize drv8323 interface
    obj->drvicHandle = DRVIC_init(&obj->drvic);

    // initialize SPI handle
    obj->spiHandle = MTR1_SPI_BASE;             //!< the SPI handle

    obj->gateEnableGPIO = MTR1_GATE_EN_GPIO;
    obj->gateCalGPIO = MTR1_GATE_CAL_GPIO;
    // BSXL8323RS_REVA
#elif defined(BSXL8323RH_REVB)
    obj->gateModeGPIO = MTR1_GATE_MODE_GPIO;
    obj->gateGainGPIO = MTR1_GATE_GAIN_GPIO;
    obj->gateCalGPIO = MTR1_GATE_CAL_GPIO;
    obj->gateEnableGPIO = MTR1_GATE_EN_GPIO;
    // BSXL8323RH_REVB
#elif defined(BSXL8353RS_REVA)
    // initialize drv8353interface
    obj->drvicHandle = DRVIC_init(&obj->drvic);

    // initialize SPI handle
    obj->spiHandle = MTR1_SPI_BASE;             //!< the SPI handle

    obj->gateEnableGPIO = MTR1_GATE_EN_GPIO;
    // BSXL8353RS_REVA
#elif defined(BSXL8316RT_REVA)
    // initialize drv8316 interface
    obj->drvicHandle = DRVIC_init(&obj->drvic);

    // initialize SPI handle
    obj->spiHandle = MTR1_SPI_BASE;             //!< the SPI handle

    obj->gateEnableGPIO = MTR1_GATE_EN_GPIO;
    obj->gateSleepGPIO = MTR1_DRV_SLEEP_GPIO;

    // enable the DRV device
    GPIO_writePin(obj->gateSleepGPIO, 0);   // 1-Active,  0-Low Power Sleep Mode

    DEVICE_DELAY_US(10.0f);                 // delay 10us

    GPIO_writePin(obj->gateEnableGPIO, 0);  // 1-Disable, 0-Enable

    DEVICE_DELAY_US(10.0f);                 // delay 10us

    GPIO_writePin(obj->gateSleepGPIO, 1);   // 1-Active,  0-Low Power Sleep Mode
    // BSXL8316RT_REVA
#elif defined(BSXL3PHGAN_REVA)
    obj->gateEnableGPIO = MTR1_GATE_EN_GPIO;
    // BSXL3PHGAN_REVA
#elif defined(HVMTRPFC_REV1P1)
    obj->gateEnableGPIO = MTR1_GATE_EN_GPIO;
    // HVMTRPFC_REV1P1
#endif  // Assign gateEnableGPIO

    // initialize QEP driver
    obj->qepHandle = MTR1_QEP_BASE;             // the QEP handle


    obj->motorNum = MTR_1;

    return(handle);
} // end of HAL_MTR1_init() function

void HAL_setParams(HAL_Handle handle)
{
    (void)handle;
    Interrupt_disableMaster();
    Wildfire_initBoardLocked();
} // end of HAL_setParams() function

void HAL_MTR_setParams(HAL_MTR_Handle handle, USER_Params *pUserParams)
{
    HAL_setNumCurrentSensors(handle,pUserParams->numCurrentSensors);
    HAL_setNumVoltageSensors(handle,pUserParams->numVoltageSensors);
    HAL_disablePWM(handle);
} // end of HAL_MTR_setParams() function

void HAL_setupADCs(HAL_Handle handle)
{
    gWildfire.blockedConfig++;
} // end of HAL_setupADCs() function

#if defined(BUFDAC_MODE)
void HAL_setupDACs(HAL_Handle handle)
{
    HAL_Obj *obj = (HAL_Obj *)handle;

    // AIO -> A0/B15/C15/DACA_OUT Analog mode selected
    GPIO_setAnalogMode(231, GPIO_ANALOG_ENABLED);

    // AIO -> A1/B7/DACB_OUT Analog mode selected
    GPIO_setAnalogMode(232, GPIO_ANALOG_ENABLED);

    // AIO -> B3/VDAC, Analog mode selected
    GPIO_setAnalogMode(242, GPIO_ANALOG_ENABLED);

    // Set DAC reference voltage.
    DAC_setReferenceVoltage(obj->bufDACHandle[0], DAC_REF_ADC_VREFHI);
    DAC_setReferenceVoltage(obj->bufDACHandle[1], DAC_REF_ADC_VREFHI);

    // Set DAC load mode.
    DAC_setLoadMode(obj->bufDACHandle[0], DAC_LOAD_SYSCLK);
    DAC_setLoadMode(obj->bufDACHandle[1], DAC_LOAD_SYSCLK);

    // Set the DAC Gain Mode
    DAC_setGainMode(obj->bufDACHandle[0], DAC_GAIN_TWO);
    DAC_setGainMode(obj->bufDACHandle[1], DAC_GAIN_TWO);

    // Enable the DAC output
    DAC_enableOutput(obj->bufDACHandle[0]);
    DAC_enableOutput(obj->bufDACHandle[1]);

    // Set the DAC shadow output
    DAC_setShadowValue(obj->bufDACHandle[0], 0U);
    DAC_setShadowValue(obj->bufDACHandle[1], 0U);

    // Delay for buffered DAC to power up.
    DEVICE_DELAY_US(500);

    return;
} // end of HAL_setupDACs() function
#endif  // BUFDAC_MODE

#if defined(MOTOR1_HALL) && defined(CMD_CAP_EN)
#error HALL and CMD_CAP can't be enabled at the same time
#elif defined(MOTOR1_HALL)
void HAL_setupCAPs(HAL_MTR_Handle handle)
{
    HAL_MTR_Obj    *obj = (HAL_MTR_Obj *)handle;
    uint16_t  cnt;

    for(cnt = 0; cnt < 3; cnt++)
    {
        // Disable ,clear all capture flags and interrupts
        ECAP_disableInterrupt(obj->capHandle[cnt],
                              (ECAP_ISR_SOURCE_CAPTURE_EVENT_1  |
                               ECAP_ISR_SOURCE_CAPTURE_EVENT_2  |
                               ECAP_ISR_SOURCE_CAPTURE_EVENT_3  |
                               ECAP_ISR_SOURCE_CAPTURE_EVENT_4  |
                               ECAP_ISR_SOURCE_COUNTER_OVERFLOW |
                               ECAP_ISR_SOURCE_COUNTER_PERIOD   |
                               ECAP_ISR_SOURCE_COUNTER_COMPARE));

        ECAP_clearInterrupt(obj->capHandle[cnt],
                            (ECAP_ISR_SOURCE_CAPTURE_EVENT_1  |
                             ECAP_ISR_SOURCE_CAPTURE_EVENT_2  |
                             ECAP_ISR_SOURCE_CAPTURE_EVENT_3  |
                             ECAP_ISR_SOURCE_CAPTURE_EVENT_4  |
                             ECAP_ISR_SOURCE_COUNTER_OVERFLOW |
                             ECAP_ISR_SOURCE_COUNTER_PERIOD   |
                             ECAP_ISR_SOURCE_COUNTER_COMPARE));

        // Disable CAP1-CAP4 register loads
        ECAP_disableTimeStampCapture(obj->capHandle[cnt]);

        // Configure eCAP
        //    Enable capture mode.
        //    One shot mode, stop capture at event 3.
        //    Set polarity of the events to rising, falling, rising edge.
        //    Set capture in time difference mode.
        //    Select input from XBAR4/5/6.
        //    Enable eCAP module.
        //    Enable interrupt.
        ECAP_stopCounter(obj->capHandle[cnt]);
        ECAP_enableCaptureMode(obj->capHandle[cnt]);

        // Sets eCAP in Capture mode
        ECAP_setCaptureMode(obj->capHandle[cnt], ECAP_CONTINUOUS_CAPTURE_MODE, ECAP_EVENT_3);

        // Sets the Capture event prescaler.
        ECAP_setEventPrescaler(obj->capHandle[cnt], 0U);

        // Sets a phase shift value count.
        ECAP_setPhaseShiftCount(obj->capHandle[cnt], 0U);

        // Sets the Capture event polarity
        ECAP_setEventPolarity(obj->capHandle[cnt], ECAP_EVENT_1, ECAP_EVNT_FALLING_EDGE);
        ECAP_setEventPolarity(obj->capHandle[cnt], ECAP_EVENT_2, ECAP_EVNT_RISING_EDGE);
        ECAP_setEventPolarity(obj->capHandle[cnt], ECAP_EVENT_3, ECAP_EVNT_FALLING_EDGE);
        ECAP_setEventPolarity(obj->capHandle[cnt], ECAP_EVENT_4, ECAP_EVNT_RISING_EDGE);

        // Configure counter reset on events
        ECAP_enableCounterResetOnEvent(obj->capHandle[cnt], ECAP_EVENT_1);
        ECAP_enableCounterResetOnEvent(obj->capHandle[cnt], ECAP_EVENT_2);
        ECAP_enableCounterResetOnEvent(obj->capHandle[cnt], ECAP_EVENT_3);
        ECAP_enableCounterResetOnEvent(obj->capHandle[cnt], ECAP_EVENT_4);

        // Configures emulation mode.
        ECAP_setEmulationMode(obj->capHandle[cnt], ECAP_EMULATION_FREE_RUN);

        // Enable counter loading with phase shift value.
        ECAP_enableLoadCounter(obj->capHandle[cnt]);

        // Configures Sync out signal mode.
        ECAP_setSyncOutMode(obj->capHandle[cnt], ECAP_SYNC_OUT_SYNCI);

        // Set up the source for sync-in pulse
        ECAP_setSyncInPulseSource(obj->capHandle[cnt], ECAP_SYNC_IN_PULSE_SRC_DISABLE);

        // Starts Time stamp counter
        ECAP_startCounter(obj->capHandle[cnt]);

        // Enables time stamp capture
        ECAP_enableTimeStampCapture(obj->capHandle[cnt]);

        // Re-arms the eCAP module
        ECAP_reArm(obj->capHandle[cnt]);
    }

    XBAR_setInputPin(INPUTXBAR_BASE, MTR1_CAP_U_XBAR, MTR1_HALL_U_GPIO);
    XBAR_setInputPin(INPUTXBAR_BASE, MTR1_CAP_V_XBAR, MTR1_HALL_V_GPIO);
    XBAR_setInputPin(INPUTXBAR_BASE, MTR1_CAP_W_XBAR, MTR1_HALL_W_GPIO);

    ECAP_selectECAPInput(obj->capHandle[0], MTR1_CAP_U_INSEL);
    ECAP_selectECAPInput(obj->capHandle[1], MTR1_CAP_V_INSEL);
    ECAP_selectECAPInput(obj->capHandle[2], MTR1_CAP_W_INSEL);

    return;
}   // HAL_setupCAPs()

void HAL_resetCAPTimeStamp(HAL_MTR_Handle handle)
{
    HAL_MTR_Obj    *obj = (HAL_MTR_Obj *)handle;
    uint16_t  cnt;

    for(cnt = 0; cnt < 3; cnt++)
    {
        ECAP_setAPWMPeriod(obj->capHandle[cnt], 0);
        ECAP_setAPWMCompare(obj->capHandle[cnt], 0x01FFFFFF);
        ECAP_setAPWMShadowPeriod(obj->capHandle[cnt], 0x01FFFFFF);
        ECAP_setAPWMShadowCompare(obj->capHandle[cnt], 0x01FFFFFF);
    }

    return;
}   // HAL_resetCAPTimeStamp()
#elif defined(CMD_CAP_EN)
void HAL_setupCAPs(HAL_MTR_Handle handle)
{
    HAL_MTR_Obj    *obj = (HAL_MTR_Obj *)handle;

    // Disable ,clear all capture flags and interrupts
    ECAP_disableInterrupt(obj->capHandle,
                          (ECAP_ISR_SOURCE_CAPTURE_EVENT_1  |
                           ECAP_ISR_SOURCE_CAPTURE_EVENT_2  |
                           ECAP_ISR_SOURCE_CAPTURE_EVENT_3  |
                           ECAP_ISR_SOURCE_CAPTURE_EVENT_4  |
                           ECAP_ISR_SOURCE_COUNTER_OVERFLOW |
                           ECAP_ISR_SOURCE_COUNTER_PERIOD   |
                           ECAP_ISR_SOURCE_COUNTER_COMPARE));

    ECAP_clearInterrupt(obj->capHandle,
                        (ECAP_ISR_SOURCE_CAPTURE_EVENT_1  |
                         ECAP_ISR_SOURCE_CAPTURE_EVENT_2  |
                         ECAP_ISR_SOURCE_CAPTURE_EVENT_3  |
                         ECAP_ISR_SOURCE_CAPTURE_EVENT_4  |
                         ECAP_ISR_SOURCE_COUNTER_OVERFLOW |
                         ECAP_ISR_SOURCE_COUNTER_PERIOD   |
                         ECAP_ISR_SOURCE_COUNTER_COMPARE));

    // Disable CAP1-CAP4 register loads
    ECAP_disableTimeStampCapture(obj->capHandle);

    // Configure eCAP
    //    Enable capture mode.
    //    One shot mode, stop capture at event 3.
    //    Set polarity of the events to rising, falling, rising edge.
    //    Set capture in time difference mode.
    //    Select input from XBAR4/5/6.
    //    Enable eCAP module.
    //    Enable interrupt.
    ECAP_stopCounter(obj->capHandle);
    ECAP_enableCaptureMode(obj->capHandle);

    // Sets the Capture event prescaler.
    ECAP_setCaptureMode(obj->capHandle, ECAP_CONTINUOUS_CAPTURE_MODE, ECAP_EVENT_4);

    // Sets a phase shift value count.
    ECAP_setPhaseShiftCount(obj->capHandle, 0U);

    // Sets the Capture event polarity
    ECAP_setEventPolarity(obj->capHandle, ECAP_EVENT_1, ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(obj->capHandle, ECAP_EVENT_2, ECAP_EVNT_RISING_EDGE);
    ECAP_setEventPolarity(obj->capHandle, ECAP_EVENT_3, ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(obj->capHandle, ECAP_EVENT_4, ECAP_EVNT_RISING_EDGE);

    // Configure counter reset on events
    ECAP_enableCounterResetOnEvent(obj->capHandle, ECAP_EVENT_1);
    ECAP_enableCounterResetOnEvent(obj->capHandle, ECAP_EVENT_2);
    ECAP_enableCounterResetOnEvent(obj->capHandle, ECAP_EVENT_3);
    ECAP_enableCounterResetOnEvent(obj->capHandle, ECAP_EVENT_4);

    // Configures emulation mode.
    ECAP_setEmulationMode(obj->capHandle, ECAP_EMULATION_FREE_RUN);

    // Enable counter loading with phase shift value.
    ECAP_enableLoadCounter(obj->capHandle);

    // Configures Sync out signal mode.
    ECAP_setSyncOutMode(obj->capHandle, ECAP_SYNC_OUT_SYNCI);

    // Set up the source for sync-in pulse
    ECAP_setSyncInPulseSource(obj->capHandle, ECAP_SYNC_IN_PULSE_SRC_DISABLE);

    // Starts Time stamp counter
    ECAP_startCounter(obj->capHandle);

    // Enables time stamp capture
    ECAP_enableTimeStampCapture(obj->capHandle);

    // Re-arms the eCAP module
    ECAP_reArm(obj->capHandle);

    XBAR_setInputPin(INPUTXBAR_BASE, MTR1_CAP_FREQ_XBAR, MTR1_CAP_FREQ_GPIO);

    ECAP_selectECAPInput(obj->capHandle, MTR1_CAP_FREQ_INSEL);

    return;
}

void HAL_resetCAPTimeStamp(HAL_MTR_Handle handle)
{
    HAL_MTR_Obj    *obj = (HAL_MTR_Obj *)handle;

    ECAP_setAPWMPeriod(obj->capHandle, 0);
    ECAP_setAPWMCompare(obj->capHandle, 0x01FFFFFF);
    ECAP_setAPWMShadowPeriod(obj->capHandle, 0x01FFFFFF);
    ECAP_setAPWMShadowCompare(obj->capHandle, 0x01FFFFFF);

    return;
}
#endif  // MOTOR1_HALL || CMD_CAP_EN

// HAL_setupCMPSSs
void HAL_setupCMPSSs(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
} // end of HAL_setupCMPSSs() function

// HAL_setupGate & HAL_enableDRV
#if defined(DRV8329AEVM_REVA)
void HAL_enableDRV(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
} // HAL_setupGate() function
// DRV8329AEVM_REVA
#elif defined(BSXL8323RS_REVA) || defined(BSXL8353RS_REVA) || \
    defined(BSXL8316RT_REVA)
void HAL_enableDRV(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
}  // end of HAL_enableDRV() function

void HAL_writeDRVData(HAL_MTR_Handle handle, DRVIC_VARS_t *drvicVars)
{
    HAL_MTR_Obj  *obj = (HAL_MTR_Obj *)handle;

    DRVIC_writeData(obj->drvicHandle, drvicVars);

    return;
}  // end of HAL_writeDRVData() function

void HAL_readDRVData(HAL_MTR_Handle handle, DRVIC_VARS_t *drvicVars)
{
    HAL_MTR_Obj  *obj = (HAL_MTR_Obj *)handle;

    DRVIC_readData(obj->drvicHandle, drvicVars);

    return;
}  // end of HAL_readDRVData() function


void HAL_setupDRVSPI(HAL_MTR_Handle handle, DRVIC_VARS_t *drvicVars)
{
    HAL_MTR_Obj  *obj = (HAL_MTR_Obj *)handle;

    DRVIC_setupSPI(obj->drvicHandle, drvicVars);

    return;
}  // end of HAL_setupDRVSPI() function

void HAL_setupGate(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
} // HAL_setupGate() function

void HAL_setupSPI(HAL_MTR_Handle handle)
{
    HAL_MTR_Obj   *obj = (HAL_MTR_Obj *)handle;

    // Must put SPI into reset before configuring it
    SPI_disableModule(obj->spiHandle);

    // SPI configuration. Use a 500kHz SPICLK and 16-bit word size, 30MHz LSPCLK
    SPI_setConfig(obj->spiHandle, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA0,
                  SPI_MODE_MASTER, 400000, 16);

    SPI_disableLoopback(obj->spiHandle);

    SPI_setEmulationMode(obj->spiHandle, SPI_EMULATION_FREE_RUN);

    SPI_enableFIFO(obj->spiHandle);
    SPI_setFIFOInterruptLevel(obj->spiHandle, SPI_FIFO_TX0, SPI_FIFO_RX0);
    SPI_setTxFifoTransmitDelay(obj->spiHandle, 0x10);

    SPI_clearInterruptStatus(obj->spiHandle, SPI_INT_TXFF);

    // Configuration complete. Enable the module.
    SPI_enableModule(obj->spiHandle);

    return;
}  // end of HAL_setupSPI() function

// BSXL8323RS_REVA || BSXL8353RS_REVA ||
// BSXL8316RT_REVA
#elif defined(BSXL8323RH_REVB)
void HAL_enableDRV(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
} // HAL_setupGate() function
// BSXL8323RH_REVB
#elif defined(BSXL3PHGAN_REVA)
void HAL_enableDRV(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
} // HAL_setupGate() function
// BSXL3PHGAN_REVA
#elif defined(HVMTRPFC_REV1P1)
void HAL_enableDRV(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
} // HAL_setupGate() function
// HVMTRPFC_REV1P1
#else
#error No HAL_setupGate or HAL_enableDRV
#endif  // HAL_setupGate & HAL_enableDRV

void HAL_setupMtrFaults(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
} // end of HAL_setupMtrFaults() function

void HAL_setupGPIOs(HAL_Handle handle)
{
    gWildfire.blockedConfig++;
}  // end of HAL_setupGPIOs() function

void HAL_setupPWMs(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
}  // end of HAL_setupPWMs() function

#if defined(EPWMDAC_MODE)
void HAL_setupPWMDACs(HAL_Handle handle,
                   const float32_t systemFreq_MHz)
{
    HAL_Obj   *obj = (HAL_Obj *)handle;

    // PWMDAC frequency = 100kHz, calculate the period for pwm
    uint16_t  period_cycles = (uint16_t)(systemFreq_MHz *
                                  (float32_t)(1000.0f / HA_PWMDAC_FREQ_KHZ));
    uint16_t  cnt;

    for(cnt = 0; cnt < 4; cnt++)
    {
        // setup the Time-Base Control Register (TBCTL)
        EPWM_setTimeBaseCounterMode(obj->pwmDACHandle[cnt],
                                    EPWM_COUNTER_MODE_UP);

        EPWM_disablePhaseShiftLoad(obj->pwmDACHandle[cnt]);

        EPWM_setPeriodLoadMode(obj->pwmDACHandle[cnt], EPWM_PERIOD_DIRECT_LOAD);

        EPWM_enableSyncOutPulseSource(obj->pwmDACHandle[cnt],
                                      EPWM_SYNC_OUT_PULSE_ON_SOFTWARE);

        EPWM_setClockPrescaler(obj->pwmDACHandle[cnt], EPWM_CLOCK_DIVIDER_1,
                                 EPWM_HSCLOCK_DIVIDER_1);

        EPWM_setCountModeAfterSync(obj->pwmDACHandle[cnt],
                                   EPWM_COUNT_MODE_UP_AFTER_SYNC);

        EPWM_setEmulationMode(obj->pwmDACHandle[cnt], EPWM_EMULATION_FREE_RUN);

        // setup the Timer-Based Phase Register (TBPHS)
        EPWM_setPhaseShift(obj->pwmDACHandle[cnt], 0);

        // setup the Time-Base Counter Register (TBCTR)
        EPWM_setTimeBaseCounter(obj->pwmDACHandle[cnt], 0);

        // setup the Time-Base Period Register (TBPRD)
        // set to zero initially
        EPWM_setTimeBasePeriod(obj->pwmDACHandle[cnt], 0);

        // setup the Counter-Compare Control Register (CMPCTL)
        EPWM_setCounterCompareShadowLoadMode(obj->pwmDACHandle[cnt],
                                             EPWM_COUNTER_COMPARE_A,
                                             EPWM_COMP_LOAD_ON_CNTR_ZERO);

        // setup the Action-Qualifier Output A Register (AQCTLA)
        EPWM_setActionQualifierAction(obj->pwmDACHandle[cnt],
                                      EPWM_AQ_OUTPUT_A,
                                      EPWM_AQ_OUTPUT_LOW,
                                      EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);

        EPWM_setActionQualifierAction(obj->pwmDACHandle[cnt],
                                      EPWM_AQ_OUTPUT_A,
                                      EPWM_AQ_OUTPUT_HIGH,
                                      EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);

        // setup the Action-Qualifier Output B Register (AQCTLB)
        EPWM_setActionQualifierAction(obj->pwmDACHandle[cnt],
                                      EPWM_AQ_OUTPUT_B,
                                      EPWM_AQ_OUTPUT_LOW,
                                      EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);

        EPWM_setActionQualifierAction(obj->pwmDACHandle[cnt],
                                      EPWM_AQ_OUTPUT_B,
                                      EPWM_AQ_OUTPUT_HIGH,
                                      EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);


        // setup the Dead-Band Generator Control Register (DBCTL)
        EPWM_setDeadBandDelayMode(obj->pwmDACHandle[cnt], EPWM_DB_RED, false);

        EPWM_setDeadBandDelayMode(obj->pwmDACHandle[cnt], EPWM_DB_FED, false);

        // setup the PWM-Chopper Control Register (PCCTL)
        EPWM_disableChopper(obj->pwmDACHandle[cnt]);

        // setup the Trip Zone Select Register (TZSEL)
        EPWM_disableTripZoneSignals(obj->pwmDACHandle[cnt], HAL_TZSEL_SIGNALS_ALL);

        // since the PWM is configured as an up/down counter, the period
        // register is set to one-half of the desired PWM period
        EPWM_setTimeBasePeriod(obj->pwmDACHandle[cnt], period_cycles);
    }

    return;
}  // end of HAL_setupPWMDACs() function
#endif  // EPWMDAC_MODE

#if defined(MOTOR1_ENC)
void HAL_setupQEP(HAL_MTR_Handle handle)
{
    gWildfire.blockedConfig++;
}
#endif  // MOTOR1_ENC

void HAL_setupSCIA(HAL_Handle halHandle)
{
    gWildfire.blockedConfig++;
}  // end of DRV_setupSci() function

void HAL_setupI2CA(HAL_Handle halHandle)
{
    gWildfire.blockedConfig++;
}  // end of HAL_setupI2CA() function

void HAL_setupTimeBaseTimer(HAL_Handle handle, const float32_t timeBaseFreq_Hz)
{
    HAL_Obj  *obj = (HAL_Obj *)handle;

    uint32_t timerPeriod = (uint32_t)((USER_SYSTEM_FREQ_MHz * 1000.0f *1000.0f) /
                                      timeBaseFreq_Hz);

    // use timer 0 for CPU usage diagnostics
    CPUTimer_setPreScaler(obj->timerHandle[0], 0);

    CPUTimer_setEmulationMode(obj->timerHandle[0],
                              CPUTIMER_EMULATIONMODE_RUNFREE);

    CPUTimer_setPeriod(obj->timerHandle[0], timerPeriod);

    CPUTimer_startTimer(obj->timerHandle[0]);

    return;
}  // end of HAL_setupTimeBaseTimer() function

void HAL_setupADCTriggerTimer(HAL_Handle handle, const float32_t adcTriggerFreq_Hz)
{
    HAL_Obj  *obj = (HAL_Obj *)handle;

    uint32_t timerPeriod = (uint32_t)((USER_SYSTEM_FREQ_MHz * 1000.0f *1000.0f) /
                                      adcTriggerFreq_Hz);

    // use timer 1 for ADC trigger
    CPUTimer_setPreScaler(obj->timerHandle[1], 0);

    CPUTimer_setEmulationMode(obj->timerHandle[1],
                              CPUTIMER_EMULATIONMODE_RUNFREE);

    CPUTimer_setPeriod(obj->timerHandle[1], timerPeriod);

    CPUTimer_enableInterrupt(obj->timerHandle[1]);

    CPUTimer_startTimer(obj->timerHandle[1]);

    return;
}  // end of HAL_setupADCTriggerTimer() function

void HAL_setupCPUUsageTimer(HAL_Handle handle)
{
    HAL_Obj  *obj = (HAL_Obj *)handle;

    // use timer 2 for CPU usage diagnostics
    CPUTimer_setPreScaler(obj->timerHandle[2], 0);

    CPUTimer_setEmulationMode(obj->timerHandle[2],
                              CPUTIMER_EMULATIONMODE_RUNFREE);

    CPUTimer_setPeriod(obj->timerHandle[2], 0xFFFFFFFF);

    CPUTimer_startTimer(obj->timerHandle[2]);

    return;
}  // end of HAL_setupCPUUsageTimer() function

#if defined(DATALOGF4_EN) || defined(DATALOGF2_EN)
void HAL_setupDMA(void)
{
    // Initializes the DMA controller to a known state
    DMA_initController();

    // Sets DMA emulation mode, Continue DMA operation
    DMA_setEmulationMode(DMA_EMULATION_FREE_RUN);

    return;
}    //end of HAL_setupDMA() function

void HAL_setupDMAforDLOG(HAL_Handle handle, const uint16_t dmaChNum,
                          const void *destAddr, const void *srcAddr)
{
    HAL_Obj *obj = (HAL_Obj *)handle;
    DMA_configAddresses(obj->dmaChHandle[dmaChNum], destAddr, srcAddr);

    // configure DMA Channel
    DMA_configBurst(obj->dmaChHandle[dmaChNum], DLOG_BURST_SIZE, 2, 2);
    DMA_configTransfer(obj->dmaChHandle[dmaChNum], DLOG_TRANSFER_SIZE, 1, 1);
    DMA_configWrap(obj->dmaChHandle[dmaChNum], 0xFFFF, 0, 0xFFFF, 0);

    DMA_configMode(obj->dmaChHandle[dmaChNum], DMA_TRIGGER_SOFTWARE,
                   DMA_CFG_ONESHOT_ENABLE | DMA_CFG_CONTINUOUS_ENABLE |
                   DMA_CFG_SIZE_32BIT);

    DMA_setInterruptMode(obj->dmaChHandle[dmaChNum], DMA_INT_AT_END);
    DMA_enableTrigger(obj->dmaChHandle[dmaChNum]);
    DMA_disableInterrupt(obj->dmaChHandle[dmaChNum]);

    return;
}    //end of HAL_setupDMAforDLOG() function
#endif  // DATALOGF4_EN || DATALOGF2_EN

void HAL_clearDataRAM(void *pMemory, uint32_t lengthMemory)
{
    uint16_t *pMemoryStart;
    uint32_t loopCount, loopLength;

    pMemoryStart = pMemory;
    loopLength = lengthMemory;

    for(loopCount = 0; loopCount < loopLength; loopCount++)
    {
        *(pMemoryStart + loopCount) = 0x0000;
    }
}   //end of HAL_clearDataRAM() function

void HAL_setMtrCMPSSDACValue(HAL_MTR_Handle handle,
                             const uint16_t dacValH, const uint16_t dacValL)
{
    (void)handle; (void)dacValH; (void)dacValL; gWildfire.blockedConfig++;
}   // end of HAL_setMtrCMPSSDACValue() function

void HAL_setTriggerPrams(HAL_PWMData_t *pPWMData, const float32_t systemFreq_MHz,
                   const float32_t deadband_us, const float32_t noiseWindow_us,
                   const float32_t adcSample_us)
{
    uint16_t deadband =  (uint16_t)(deadband_us * systemFreq_MHz);
    uint16_t noiseWindow =  (uint16_t)(noiseWindow_us * systemFreq_MHz);
    uint16_t adcSample =  (uint16_t)(adcSample_us * systemFreq_MHz);

    pPWMData->deadband = deadband;
    pPWMData->noiseWindow = noiseWindow;
    pPWMData->adcSample = adcSample;

    pPWMData->minCMPValue = deadband + noiseWindow + adcSample;

    return;
}   // end of HAL_setTriggerPrams() function










bool HAL_MTR_setGateDriver(HAL_MTR_Handle handle)
{
    (void)handle; gWildfire.blockedConfig++; return true;
}


// end of file
