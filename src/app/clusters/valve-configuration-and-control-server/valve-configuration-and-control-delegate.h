/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <app-common/zap-generated/cluster-enums.h>
#include <app/data-model/Nullable.h>
#include <app/util/basic-types.h>
#include <lib/core/CHIPError.h>

namespace chip {
namespace app {
namespace Clusters {
namespace ValveConfigurationAndControl {

// TODO: Change this to a level delegate and a non-level delegate.
enum DelegateType
{
    kBase,
    kLevel,
    kNonLevel,
};
class DelegateBase
{
public:
    DelegateBase(){};
    virtual ~DelegateBase() = default;

    // This delegate function will be called only for valve implementations that support the LVL feature.
    // Delegates for valves that do not support the LVL feature return CHIP_ERROR_NOT_IMPLEMENTED.
    // When this function is called, the delegate should set the valve to the target level, or begin the async process of opening
    // the valve to the desired level.
    // If the valve is able to be opened (success)
    //  - the delegate should set currentLevel
    //    - If the valve is fully open to the target level, currentLevel should be set to the targetLevel
    //    - If the valve is not fully opened, the delegate should set the currentLevel to the current valve level and the caller
    //    will continue to query the valve level until the target level is reached or the valve is closed.
    //  - A valve fault may be returned even if the Open command is successful, if the fault did not prevent the valve from safely
    //  opening
    //  - return CHIP_NO_ERROR
    // If the valve cannot be safely opened (failure)
    //  - The delegate should set the valveFault parameter to indicate the reason for the failure (if applicable)
    //  - The delegate should return a CHIP_ERROR_INTERNAL
    virtual CHIP_ERROR HandleOpenValve(const Percent targetLevel, Percent & currentLevel,
                                       BitMask<ValveFaultBitmap> & valveFault) = 0;

    // This delegate function will be called only for valve implementations that DO NOT support the LVL feature.
    // Delegates for valves that support the LVL feature should return CHIP_ERROR_NOT_IMPLEMENTED.
    // When this function is called, the delegate should open the valve, or begin the async process of opening the valve.
    // If the valve is able to be opened (success)
    //    - If the valve is fully open, currentState should be set to kOpen
    //    - If the valve is not fully opened, the delegate should set the currentState to kTransitioning and the caller
    //    will continue to query the valve level until the target level is reached or the valve is closed.
    //  - A valve fault may be returned even if the Open command is successful, if the fault did not prevent the valve from safely
    //  opening
    //  - return CHIP_NO_ERROR
    // If the valve cannot be safely opened (failure)
    //  - The delegate should set the valveFault parameter to indicate the reason for the failure (if applicable)
    //  - The delegate should return a CHIP_ERROR_INTERNAL
    virtual CHIP_ERROR HandleOpenValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault) = 0;

    // This delegate function will be called only for valve implementations that support the LVL feature.
    virtual Percent GetCurrentValveLevel() = 0;
    // This delegate function will be called only for valve implementations that do not support the LVL feature.
    virtual ValveStateEnum GetCurrentValveState() = 0;

    // This delegate function will be called when the valve needs to be closed either due to an explicit command or
    // from the expiration of the open duration. This function will be called for valves that support the LVL feature.
    // Delegates for valves that do not support the LVL feature should return CHIP_ERROR_NOT_IMPLEMENTED.
    // When this function is called, the delegate should close the valve, or begin the async process of closing.
    // If the valve is able to be closed (success)
    //  - the delegate should set currentLevel
    //    - If the valve is fully closed, currentLevel should be set to 0
    //    - If the valve is not fully closed, the delegate should set the currentLevel to the current valve level and the caller
    //    will continue to query the valve level until the valve reaches 0 or a command overrides.
    //  - A valve fault may be returned even if the Open command is successful, if the fault did not prevent the valve from safely
    //  closing
    //  - return CHIP_NO_ERROR
    // If the valve cannot be closed (failure)
    //  - The delegate should set the valveFault parameter to indicate the reason for the failure (if applicable)
    //  - The delegate should return a CHIP_ERROR_INTERNAL
    virtual CHIP_ERROR HandleCloseValve(Percent & currentLevel, BitMask<ValveFaultBitmap> & valveFault) = 0;

    // This delegate function will be called when the valve needs to be closed either due to an explicit command or
    // from the expiration of the open duration.
    // This delegate function will be called only for valve implementations that DO NOT support the LVL feature.
    // Delegates for valves that support the LVL feature should return CHIP_ERROR_NOT_IMPLEMENTED.
    // When this function is called, the delegate should close the valve, or begin the async process of closing.
    // If the valve is able to be closed (success)
    //    - If the valve is fully closed, currentState should be set to kClosed
    //    - If the valve is not fully opened, the delegate should set the currentLevel to the current valve level and the caller
    //    will continue to query the valve level until the target level is reached or the valve is closed.
    //  - A valve fault may be returned even if the Open command is successful, if the fault did not prevent the valve from safely
    //  opening
    //  - return CHIP_NO_ERROR
    // If the valve cannot be safely opened (failure)
    //  - The delegate should set the valveFault parameter to indicate the reason for the failure (if applicable)
    //  - The delegate should return a CHIP_ERROR_INTERNAL
    virtual CHIP_ERROR HandleCloseValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault) = 0;

    virtual DelegateType GetDelegateType() { return DelegateType::kBase; };
};

class LevelControlDelegate : public DelegateBase
{
    virtual CHIP_ERROR HandleOpenValve(const Percent targetLevel, Percent & currentLevel,
                                       BitMask<ValveFaultBitmap> & valveFault) = 0;

    virtual Percent GetCurrentValveLevel()                                                              = 0;
    virtual CHIP_ERROR HandleCloseValve(Percent & currentLevel, BitMask<ValveFaultBitmap> & valveFault) = 0;

    // Final overrides - the driver should not implement these classes
    CHIP_ERROR HandleOpenValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault) final
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }
    CHIP_ERROR HandleCloseValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault) final
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }

    ValveStateEnum GetCurrentValveState() final { return ValveStateEnum::kUnknownEnumValue; }
    DelegateType GetDelegateType() final { return DelegateType::kLevel; };
};

class NonLevelControlDelegate : public DelegateBase
{
    virtual CHIP_ERROR HandleOpenValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault)  = 0;
    virtual ValveStateEnum GetCurrentValveState()                                                              = 0;
    virtual CHIP_ERROR HandleCloseValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault) = 0;

    CHIP_ERROR HandleOpenValve(const Percent targetLevel, Percent & currentLevel, BitMask<ValveFaultBitmap> & valveFault) final
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }
    CHIP_ERROR HandleCloseValve(Percent & currentLevel, BitMask<ValveFaultBitmap> & valveFault) final
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }

    Percent GetCurrentValveLevel() final { return 0; }
    DelegateType GetDelegateType() final { return DelegateType::kNonLevel; };
};

} // namespace ValveConfigurationAndControl
} // namespace Clusters
} // namespace app
} // namespace chip
