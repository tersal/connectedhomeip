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
 *
 */

#include "valve-configuration-and-control-cluster.h"

#include <app/persistence/AttributePersistence.h>
#include <app/server-cluster/AttributeListBuilder.h>
#include <clusters/ValveConfigurationAndControl/Commands.h>
#include <lib/support/CodeUtils.h>

#include <platform/CHIPDeviceLayer.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ValveConfigurationAndControl;
using chip::Protocols::InteractionModel::Status;

ValveConfigurationAndControlCluster::ValveConfigurationAndControlCluster(EndpointId endpoint, BitFlags<ValveConfigurationAndControl::Feature> features, OptionalAttributeSet optionalAttributeSet) :
    DefaultServerCluster( {endpoint, ValveConfigurationAndControl::Id }), mFeatures(features), mOptionalAttributeSet(optionalAttributeSet)
{
    
}

CHIP_ERROR ValveConfigurationAndControlCluster::Attributes(const ConcreteClusterPath & path, ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder)
{
    AttributeListBuilder listBuilder(builder);

    const bool isDefaultOpenLevelSupported = (mFeatures.Has(Feature::kLevel) && mOptionalAttributeSet.IsSet(Attributes::DefaultOpenLevel::Id));
    const bool isLevelStepSupported = (mFeatures.Has(Feature::kLevel) && mOptionalAttributeSet.IsSet(Attributes::LevelStep::Id));

    AttributeListBuilder::OptionalAttributeEntry optionalAttributeEntries[] = {
        { mFeatures.Has(Feature::kTimeSync), Attributes::AutoCloseTime::kMetadataEntry },
        { mFeatures.Has(Feature::kLevel), Attributes::CurrentLevel::kMetadataEntry },
        { mFeatures.Has(Feature::kLevel), Attributes::TargetLevel::kMetadataEntry },
        { isDefaultOpenLevelSupported, Attributes::DefaultOpenLevel::kMetadataEntry },
        { mOptionalAttributeSet.IsSet(Attributes::ValveFault::Id), Attributes::ValveFault::kMetadataEntry },
        { isLevelStepSupported, Attributes::LevelStep::kMetadataEntry }
    };

    return listBuilder.Append(Span(ValveConfigurationAndControl::Attributes::kMandatoryMetadata), Span(optionalAttributeEntries));
}


    
DataModel::ActionReturnStatus ValveConfigurationAndControlCluster::ReadAttribute(const DataModel::ReadAttributeRequest & request, AttributeValueEncoder & encoder)
{
    switch(request.path.mAttributeId)
    {
    case ValveConfigurationAndControl::Attributes::FeatureMap::Id:
    {
        return encoder.Encode(mFeatures);
    }
    case ValveConfigurationAndControl::Attributes::ClusterRevision::Id:
    {
        return encoder.Encode(ValveConfigurationAndControl::kRevision);
    }
    case ValveConfigurationAndControl::Attributes::OpenDuration::Id:
    {
        return encoder.Encode(mOpenDuration);
    }
    case ValveConfigurationAndControl::Attributes::DefaultOpenDuration::Id:
    {
        return encoder.Encode(mDefaultOpenDuration);
    }
    case ValveConfigurationAndControl::Attributes::AutoCloseTime::Id:
    {
        return encoder.Encode(mAutoCloseTime);
    }
    case ValveConfigurationAndControl::Attributes::RemainingDuration::Id:
    {
        return encoder.Encode(mRemainingDuration.value());
    }
    case ValveConfigurationAndControl::Attributes::CurrentState::Id:
    {
        return encoder.Encode(mCurrentState);
    }
    case ValveConfigurationAndControl::Attributes::TargetState::Id:
    {
        return encoder.Encode(mTargetState);
    }
    case ValveConfigurationAndControl::Attributes::CurrentLevel::Id:
    {
        return encoder.Encode(mCurrentLevel);
    }
    case ValveConfigurationAndControl::Attributes::TargetLevel::Id:
    {
        return encoder.Encode(mTargetLevel);
    }
    case ValveConfigurationAndControl::Attributes::DefaultOpenLevel::Id:
    {
        return encoder.Encode(mDefaultOpenLevel);
    }
    case ValveConfigurationAndControl::Attributes::ValveFault::Id:
    {
        return encoder.Encode(mValveFault);
    }
    case ValveConfigurationAndControl::Attributes::LevelStep::Id:
    {
        return encoder.Encode(mLevelStep);
    }
    default:
        return Status::UnsupportedAttribute;
    }
}

DataModel::ActionReturnStatus ValveConfigurationAndControlCluster::WriteAttribute(const DataModel::WriteAttributeRequest & request, AttributeValueDecoder & decoder)
{
    return NotifyAttributeChangedIfSuccess(request.path.mAttributeId, WriteImpl(request, decoder));
}

DataModel::ActionReturnStatus ValveConfigurationAndControlCluster::WriteImpl(const DataModel::WriteAttributeRequest & request, AttributeValueDecoder & decoder)
{
    if(request.path.mAttributeId == ValveConfigurationAndControl::Attributes::DefaultOpenDuration::Id)
    {
        DataModel::Nullable<uint32_t> defaultOpenDuration;
        ReturnErrorOnFailure(decoder.Decode(defaultOpenDuration));
        VerifyOrReturnValue(defaultOpenDuration != mDefaultOpenDuration, DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp);
        mDefaultOpenDuration = defaultOpenDuration;
        return mContext->attributeStorage.WriteValue(request.path, { reinterpret_cast<const uint8_t *>(&mDefaultOpenDuration), sizeof(mDefaultOpenDuration) });
    }

    if(request.path.mAttributeId == ValveConfigurationAndControl::Attributes::DefaultOpenLevel::Id)
    {
        Percent defaultOpenLvel;
        ReturnErrorOnFailure(decoder.Decode(defaultOpenLvel));
        VerifyOrReturnValue(defaultOpenLvel != mDefaultOpenLevel, DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp);
        mDefaultOpenLevel = defaultOpenLvel;
        return mContext->attributeStorage.WriteValue(request.path, { reinterpret_cast<const uint8_t *>(&mDefaultOpenLevel), sizeof(mDefaultOpenLevel) });
    }

    return Protocols::InteractionModel::Status::UnsupportedWrite;
}
//Commands
std::optional<DataModel::ActionReturnStatus> ValveConfigurationAndControlCluster::HandleCloseCommand(const DataModel::InvokeRequest & request, TLV::TLVReader & input_arguments, CommandHandler * handler)
{
    DeviceLayer::SystemLayer().CancelTimer(HandleUpdateRemainingDuration, this);
    return HandleCloseInternal();
}

std::optional<DataModel::ActionReturnStatus> ValveConfigurationAndControlCluster::HandleOpenCommand(const DataModel::InvokeRequest & request, TLV::TLVReader & input_arguments, CommandHandler * handler)
{
    // openDuration
    // - if this is omitted, fall back to defaultOpenDuration
    // - if this is NULL, remaining duration is NULL
    // - if this is a value, use that value
    // - if remaining duration is not null and TS is supported, set the autoCloseTime as appropriate
    // targetLevel
    // - if LVL is not supported
    //   - if this is omitted, that's correct
    //   - if this is supplied return error
    // - if LVL is supported
    //   - if this value is not supplied, use defaultOpenLevel if supported, otherwise 100
    //   - if this value is supplied, check against levelStep, error if not OK, otherwise set targetLevel
    Commands::Open::DecodableType commandData;
    ReturnErrorOnFailure(commandData.Decode(input_arguments));

    if(!mFeatures.Has(Feature::kLevel) && commandData.targetLevel.HasValue())
    {
        return Status::ConstraintError;
    }

    if(mFeatures.Has(Feature::kLevel))
    {
        ReturnValueOnFailure(HandleOpenLevel(commandData.targetLevel), Status::Failure);
    }
    else
    {
        ReturnValueOnFailure(HandleOpenNoLevel(), Status::Failure);
    }
    
    DataModel::Nullable<ElapsedS> realOpenDuration = commandData.openDuration.ValueOr(mDefaultOpenDuration);

    SaveAndReportIfChanged(mOpenDuration, realOpenDuration, Attributes::OpenDuration::Id);
    mDurationStarted = System::SystemClock().GetMonotonicMilliseconds64();
    HandleUpdateRemainingDurationInternal();

    return Status::Success;
}

void ValveConfigurationAndControlCluster::HandleUpdateRemainingDuration(System::Layer * systemLayer, void * context)
{
    auto * logic = static_cast<ValveConfigurationAndControlCluster *>(context);
    logic->HandleUpdateRemainingDurationInternal();
}

void ValveConfigurationAndControlCluster::HandleUpdateRemainingDurationInternal()
{
    // Start by cancelling the timer in case this was called from a command handler
    // We will start a new timer if required.
    DeviceLayer::SystemLayer().CancelTimer(HandleUpdateRemainingDuration, this);

    if (mOpenDuration.IsNull())
    {
        // I think this might be an error state - if openDuration is NULL, this timer shouldn't be on.
        SetRemainingDuration(DataModel::NullNullable);
        return;
    }

    // Setup a new timer to either send the next report or handle the close operation
    System::Clock::Milliseconds64 now      = System::SystemClock().GetMonotonicMilliseconds64();
    System::Clock::Seconds64 openDurationS = System::Clock::Seconds64(mOpenDuration.ValueOr(0));
    System::Clock::Milliseconds64 closeTimeMs =
        mDurationStarted + std::chrono::duration_cast<System::Clock::Milliseconds64>(openDurationS);
    if (now >= closeTimeMs)
    {
        // Time's up, close the valve. Close handles setting the open and remaining duration
        HandleCloseInternal();
        return;
    }
    System::Clock::Milliseconds64 remainingMs     = closeTimeMs - now;
    System::Clock::Milliseconds64 nextReportTimer = GetNextReportTimeForRemainingDuration() - now;

    System::Clock::Milliseconds64 nextTimerTime = std::min(nextReportTimer, remainingMs);
    DeviceLayer::SystemLayer().StartTimer(std::chrono::duration_cast<System::Clock::Timeout>(nextTimerTime),
                                          HandleUpdateRemainingDuration, this);

    auto remainingS = std::chrono::round<System::Clock::Seconds32>(remainingMs);
    SetRemainingDuration(DataModel::Nullable<ElapsedS>(remainingS.count()));
}

System::Clock::Milliseconds64 ValveConfigurationAndControlCluster::GetNextReportTimeForRemainingDuration()
{
    return std::chrono::duration_cast<System::Clock::Milliseconds64>(mRemainingDuration.GetLastReportTime()) +
        kRemainingDurationReportRate;
}


CHIP_ERROR ValveConfigurationAndControlCluster::HandleCloseInternal()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    BitMask<ValveFaultBitmap> faults;
    if (mFeatures.Has(Feature::kLevel))
    {
        Percent currentLevel;
        
        SaveAndReportIfChanged(mTargetLevel, static_cast<u_char>(0), Attributes::TargetLevel::Id);
        SaveAndReportIfChanged(mTargetState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kClosed), Attributes::TargetState::Id);
        SaveAndReportIfChanged(mCurrentState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning), Attributes::CurrentState::Id);

        if(mDelegate != nullptr)
        {
            err = mDelegate->HandleCloseValve(currentLevel, faults);
        }
        
        if (err == CHIP_NO_ERROR)
        {
            SaveAndReportIfChanged(mCurrentLevel, DataModel::Nullable<Percent>(currentLevel), Attributes::CurrentLevel::Id);
            if (currentLevel == 0)
            {
                SaveAndReportIfChanged(mCurrentState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kClosed), Attributes::CurrentState::Id);
                SaveAndReportIfChanged(mTargetState, DataModel::NullNullable, Attributes::TargetState::Id);
                SaveAndReportIfChanged(mTargetLevel, DataModel::NullNullable, Attributes::TargetLevel::Id);
            }
            else
            {
                // TODO: start a timer here to query the delegate?
            }
        }
    }
    else
    {
        ValveStateEnum state;
        SaveAndReportIfChanged(mTargetState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kClosed), Attributes::TargetState::Id);
        SaveAndReportIfChanged(mCurrentState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning), Attributes::CurrentState::Id);

        if(mDelegate != nullptr)
        {
            err = mDelegate->HandleCloseValve(state, faults);
        }
        
        if (err == CHIP_NO_ERROR)
        {
            SaveAndReportIfChanged(mCurrentState, state, Attributes::CurrentState::Id);
        }
    }
    // If there was an error, we know nothing about the current state
    if (err != CHIP_NO_ERROR)
    {
        SaveAndReportIfChanged(mCurrentLevel, DataModel::NullNullable, Attributes::CurrentLevel::Id);
        SaveAndReportIfChanged(mCurrentState, DataModel::NullNullable, Attributes::CurrentState::Id);
    }

    SaveAndReportIfChanged(mValveFault, faults, Attributes::ValveFault::Id);
    SaveAndReportIfChanged(mOpenDuration, DataModel::NullNullable, Attributes::OpenDuration::Id);
    SetRemainingDuration(DataModel::NullNullable);
    SaveAndReportIfChanged(mTargetLevel, DataModel::NullNullable, Attributes::TargetLevel::Id);
    SaveAndReportIfChanged(mTargetState, DataModel::NullNullable, Attributes::TargetState::Id);
    SaveAndReportIfChanged(mAutoCloseTime, DataModel::NullNullable, Attributes::AutoCloseTime::Id);
    return err;
}

CHIP_ERROR ValveConfigurationAndControlCluster::SetRemainingDuration(const DataModel::Nullable<ElapsedS> & remainingDuration)
{
    System::Clock::Milliseconds64 now = System::SystemClock().GetMonotonicMilliseconds64();
    AttributeDirtyState dirtyState    = mRemainingDuration.SetValue(
        remainingDuration, now, mRemainingDuration.GetPredicateForSufficientTimeSinceLastDirty(kRemainingDurationReportRate));
    if (dirtyState == AttributeDirtyState::kMustReport)
    {
        NotifyAttributeChanged(Attributes::RemainingDuration::Id);
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR ValveConfigurationAndControlCluster::HandleOpenNoLevel()
{
    // This function should only be called for devices that do not support the level feature.
    VerifyOrReturnError(!mFeatures.Has(Feature::kLevel), CHIP_ERROR_INTERNAL);

    ValveStateEnum returnedState                 = ValveStateEnum::kUnknownEnumValue;
    BitMask<ValveFaultBitmap> returnedValveFault = 0;

    // Per the spec, set these to transitioning regardless
    SaveAndReportIfChanged(mTargetState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kOpen), Attributes::TargetState::Id);
    SaveAndReportIfChanged(mCurrentState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning), Attributes::CurrentState::Id);

    CHIP_ERROR err = CHIP_NO_ERROR;

    if(mDelegate != nullptr)
    {
        err = mDelegate->HandleOpenValve(returnedState, returnedValveFault);
    }
    
    if (mOptionalAttributeSet.IsSet(Attributes::ValveFault::Id))
    {
        SaveAndReportIfChanged(mValveFault, returnedValveFault, Attributes::ValveFault::Id);
    }
    if (err != CHIP_NO_ERROR)
    {
        // TODO: How should the target and current be set in this case?
        return err;
    }

    if (returnedState == ValveStateEnum::kOpen)
    {
        SaveAndReportIfChanged(mTargetState, DataModel::NullNullable, Attributes::TargetState::Id);
        SaveAndReportIfChanged(mCurrentState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kOpen), Attributes::CurrentState::Id);
    }
    else
    {
        // TODO: Need to start a timer to continue querying the device for updates. Or just let the delegate handle this?
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR ValveConfigurationAndControlCluster::HandleOpenLevel(const Optional<Percent> & targetLevel)
{
    Percent realTargetLevel;
    Percent returnedCurrentLevel                 = 0;
    BitMask<ValveConfigurationAndControl::ValveFaultBitmap> returnedValveFault = 0;
    ReturnErrorOnFailure(GetRealTargetLevel(targetLevel, realTargetLevel));

    CHIP_ERROR err = CHIP_NO_ERROR;

    if(mDelegate != nullptr)
    {
        err = mDelegate->HandleOpenValve(realTargetLevel, returnedCurrentLevel, returnedValveFault);
    }

    if(mOptionalAttributeSet.IsSet(Attributes::ValveFault::Id))
    {
        SaveAndReportIfChanged(mValveFault, returnedValveFault, Attributes::ValveFault::Id);
    }

    if(err != CHIP_NO_ERROR)
    {
        return err;
    }

    SaveAndReportIfChanged(mTargetLevel, realTargetLevel, Attributes::TargetLevel::Id);
    SaveAndReportIfChanged(mCurrentLevel, returnedCurrentLevel, Attributes::CurrentLevel::Id);
    SaveAndReportIfChanged(mTargetState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kOpen), Attributes::TargetState::Id);
    SaveAndReportIfChanged(mCurrentState, DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning), Attributes::CurrentState::Id);

    if (returnedCurrentLevel == realTargetLevel)
    {
        SaveAndReportIfChanged(mTargetLevel, DataModel::NullNullable, Attributes::TargetLevel::Id);
        SaveAndReportIfChanged(mCurrentLevel, realTargetLevel, Attributes::CurrentLevel::Id);
        SaveAndReportIfChanged(mTargetState, DataModel::NullNullable, Attributes::TargetState::Id);
        SaveAndReportIfChanged(mCurrentState, ValveStateEnum::kOpen, Attributes::CurrentState::Id);
    }
    else
    {
        // TODO: Need to start a timer to continue querying the device for updates. Or just let the delegate handle this?
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR ValveConfigurationAndControlCluster::GetRealTargetLevel(const Optional<Percent> & targetLevel, Percent & realTargetLevel)
{
    if(!targetLevel.HasValue())
    {
        if(mOptionalAttributeSet.IsSet(Attributes::DefaultOpenLevel::Id))
        {
            realTargetLevel = mDefaultOpenLevel;
            return CHIP_NO_ERROR;
        }
        realTargetLevel = 100u;
        return CHIP_NO_ERROR;
    }

    // targetLevel has a value
    VerifyOrReturnError(ValueCompliesWithLevelStep(targetLevel.Value()), CHIP_ERROR_INVALID_ARGUMENT);
    realTargetLevel = targetLevel.Value();
    return CHIP_NO_ERROR;
}

bool ValveConfigurationAndControlCluster::ValueCompliesWithLevelStep(const uint8_t value)
{
    if (mOptionalAttributeSet.IsSet(Attributes::LevelStep::Id))
    {
        if ((value != 100u) && ((value % mLevelStep) != 0))
        {
            return false;
        }
    }
    return true;
}

CHIP_ERROR ValveConfigurationAndControlCluster::SetDelegate(DelegateBase * delegate)
{
    mDelegate = delegate;
    return CHIP_NO_ERROR;
}


std::optional<DataModel::ActionReturnStatus> ValveConfigurationAndControlCluster::InvokeCommand(const DataModel::InvokeRequest & request,
                                                            TLV::TLVReader & input_arguments, CommandHandler * handler)
{
    switch (request.path.mCommandId)
    {
    case ValveConfigurationAndControl::Commands::Close::Id:
        return HandleCloseCommand(request, input_arguments, handler);
    case ValveConfigurationAndControl::Commands::Open::Id:
    {
        ChipLogError(Zcl, "Handling Open command");
        return HandleOpenCommand(request, input_arguments, handler);
    }
    default:
        return Protocols::InteractionModel::Status::UnsupportedCommand;
    }
}

CHIP_ERROR ValveConfigurationAndControlCluster::AcceptedCommands(const ConcreteClusterPath & path,
                                                                ReadOnlyBufferBuilder<DataModel::AcceptedCommandEntry> & builder)
{
    static constexpr DataModel::AcceptedCommandEntry kAcceptedCommands[] = {
        ValveConfigurationAndControl::Commands::Open::kMetadataEntry,
        ValveConfigurationAndControl::Commands::Close::kMetadataEntry
    };

    return builder.ReferenceExisting(kAcceptedCommands);
}