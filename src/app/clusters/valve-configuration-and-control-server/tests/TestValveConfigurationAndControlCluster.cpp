/*
 *    Copyright (c) 2025 Project CHIP Authors
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

#include <pw_unit_test/framework.h>

#include <app/clusters/testing/AttributeTesting.h>
#include <app/clusters/testing/ClusterTester.h>
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-cluster.h>
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-delegate.h>
#include <app/server-cluster/AttributeListBuilder.h>
#include <app/server-cluster/testing/TestServerClusterContext.h>
#include <clusters/ValveConfigurationAndControl/Attributes.h>
#include <clusters/ValveConfigurationAndControl/Commands.h>
#include <clusters/ValveConfigurationAndControl/Events.h>
#include <clusters/ValveConfigurationAndControl/Metadata.h>

namespace {

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ValveConfigurationAndControl;
using namespace chip::app::Clusters::ValveConfigurationAndControl::Attributes;
using namespace chip::Test;

class DummyDelegate : public Delegate
{
public:
    DummyDelegate(){};
    ~DummyDelegate() override = default;

    DataModel::Nullable<chip::Percent> HandleOpenValve(DataModel::Nullable<chip::Percent> level) override
    {
        return DataModel::Nullable<chip::Percent>();
    }

    CHIP_ERROR HandleCloseValve() override { return CHIP_NO_ERROR; }

    void HandleRemainingDurationTick(uint32_t duration) override {}
};

class DummyTimeSyncTracker : public TimeSyncTracker
{
public:
    bool IsTimeSyncClusterSupported() override { return false; }
    bool IsValidUTCTime() override { return false; }
};

struct TestValveConfigurationAndControlCluster : public ::testing::Test
{
    static void SetUpTestSuite() { ASSERT_EQ(chip::Platform::MemoryInit(), CHIP_NO_ERROR); }
    static void TearDownTestSuite() { chip::Platform::MemoryShutdown(); }

    TestValveConfigurationAndControlCluster() {}

    chip::Test::TestServerClusterContext testContext;
    DummyDelegate delegate;
    DummyTimeSyncTracker timeSyncTracker;
};

TEST_F(TestValveConfigurationAndControlCluster, DummyTest)
{
    const BitFlags<Feature> features{ 0U };
    ValveConfigurationAndControlCluster valveCluster(kRootEndpointId, features,
                                                     ValveConfigurationAndControlCluster::OptionalAttributeSet(),
                                                     &timeSyncTracker);
    valveCluster.SetDelegate(&delegate);

    ASSERT_EQ(valveCluster.Startup(testContext.Get()), CHIP_NO_ERROR);

    uint16_t revision{};
    ClusterTester tester(valveCluster);
    ASSERT_EQ(tester.ReadAttribute(ClusterRevision::Id, revision), CHIP_NO_ERROR);
    ASSERT_EQ(revision, kRevision);

    valveCluster.Shutdown();
}

} // namespace
