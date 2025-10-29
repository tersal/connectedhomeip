/**
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

/**
 * @file API declarations for valve configuration and control cluster.
 */

#pragma once

#include "valve-configuration-and-control-delegate.h"

#include <clusters/ValveConfigurationAndControl/Metadata.h>
#include <app/server-cluster/DefaultServerCluster.h>
#include <app/server-cluster/OptionalAttributeSet.h>
#include <app/cluster-building-blocks/QuieterReporting.h>
#include <system/SystemLayer.h>

using namespace chip::app::Clusters::ValveConfigurationAndControl;
namespace chip::app::Clusters {
class ValveConfigurationAndControlCluster : public DefaultServerCluster {
public:
    using OptionalAttributeSet = chip::app::OptionalAttributeSet<
        ValveConfigurationAndControl::Attributes::DefaultOpenLevel::Id, ValveConfigurationAndControl::Attributes::ValveFault::Id,
        ValveConfigurationAndControl::Attributes::LevelStep::Id>;
    
    ValveConfigurationAndControlCluster(EndpointId endpointId, BitFlags<ValveConfigurationAndControl::Feature> features, OptionalAttributeSet optionalAttributeSet);

    // Server cluster implementation
    DataModel::ActionReturnStatus ReadAttribute(const DataModel::ReadAttributeRequest & request,
                                                AttributeValueEncoder & encoder) override;
    DataModel::ActionReturnStatus WriteAttribute(const DataModel::WriteAttributeRequest & request,
                                                 AttributeValueDecoder & decoder) override;
    CHIP_ERROR Attributes(const ConcreteClusterPath & path, ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder) override;
    CHIP_ERROR AcceptedCommands(const ConcreteClusterPath & path,
                                ReadOnlyBufferBuilder<DataModel::AcceptedCommandEntry> & builder) override;
    std::optional<DataModel::ActionReturnStatus> InvokeCommand(const DataModel::InvokeRequest & request,
                                                               TLV::TLVReader & input_arguments, CommandHandler * handler) override;

    // Weird functions
    CHIP_ERROR SetDelegate(DelegateBase * delegate);
    static void HandleUpdateRemainingDuration(System::Layer * systemLayer, void * context); 

private:
    DataModel::ActionReturnStatus WriteImpl(const DataModel::WriteAttributeRequest & request, AttributeValueDecoder & decoder);
    std::optional<DataModel::ActionReturnStatus> HandleOpenCommand(const DataModel::InvokeRequest & request, TLV::TLVReader & input_arguments, CommandHandler * handler);
    std::optional<DataModel::ActionReturnStatus> HandleCloseCommand(const DataModel::InvokeRequest & request, TLV::TLVReader & input_arguments, CommandHandler * handler);
    CHIP_ERROR HandleOpenLevel(const Optional<Percent> & targetLevel);
    CHIP_ERROR HandleOpenNoLevel();
    CHIP_ERROR GetRealTargetLevel(const Optional<Percent> & targetLevel, Percent & realTargetLevel);
    bool ValueCompliesWithLevelStep(const uint8_t value);
    void HandleUpdateRemainingDurationInternal();
    CHIP_ERROR SetRemainingDuration(const DataModel::Nullable<ElapsedS> & remainingDuration);
    CHIP_ERROR HandleCloseInternal();
    System::Clock::Milliseconds64 GetNextReportTimeForRemainingDuration();

    template <typename T, typename U>
    void SaveAndReportIfChanged(T& currentValue, const U & newValue, chip::AttributeId attributeId)
    {
        if(currentValue != newValue)
        {
            currentValue = newValue;
            NotifyAttributeChanged(attributeId);
        }
    }

    //Attributes
    DataModel::Nullable<uint32_t> mOpenDuration = DataModel::NullNullable;
    DataModel::Nullable<uint32_t> mDefaultOpenDuration = DataModel::NullNullable;
    DataModel::Nullable<uint32_t> mAutoCloseTime = DataModel::NullNullable;
    QuieterReportingAttribute<uint32_t> mRemainingDuration = QuieterReportingAttribute<uint32_t>();
    DataModel::Nullable<ValveConfigurationAndControl::ValveStateEnum> mCurrentState = DataModel::NullNullable;
    DataModel::Nullable<ValveConfigurationAndControl::ValveStateEnum> mTargetState = DataModel::NullNullable;
    DataModel::Nullable<Percent> mCurrentLevel = DataModel::NullNullable;
    DataModel::Nullable<Percent> mTargetLevel = DataModel::NullNullable;
    Percent mDefaultOpenLevel = 100u;
    BitMask<ValveConfigurationAndControl::ValveFaultBitmap> mValveFault = 0u;
    uint8_t mLevelStep = 1u;
    const BitFlags<ValveConfigurationAndControl::Feature> mFeatures;
    const OptionalAttributeSet mOptionalAttributeSet;
    DelegateBase * mDelegate;
    // Check these things
    System::Clock::Milliseconds64 mDurationStarted = System::Clock::Milliseconds64(0);
    const System::Clock::Milliseconds64 kRemainingDurationReportRate =
        std::chrono::duration_cast<System::Clock::Milliseconds64>(System::Clock::Seconds64(1));
};
}