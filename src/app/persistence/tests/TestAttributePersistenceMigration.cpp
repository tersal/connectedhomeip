/*
 *    Copyright (c) 2025 Project CHIP Authors
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

#include <app/AttributeValueDecoder.h>
#include <app/ConcreteAttributePath.h>
#include <app/DefaultSafeAttributePersistenceProvider.h>
#include <app/data-model-provider/tests/WriteTesting.h>
#include <app/persistence/AttributePersistence.h>
#include <app/persistence/DefaultAttributePersistenceProvider.h>
#include <app/persistence/DefaultAttributePersistenceProvider.h>
#include <app/persistence/String.h>
#include <clusters/TimeFormatLocalization/Enums.h>
#include <clusters/TimeFormatLocalization/EnumsCheck.h>
#include <lib/core/CHIPError.h>
#include <lib/core/StringBuilderAdapters.h>
#include <lib/support/DefaultStorageKeyAllocator.h>
#include <lib/support/Span.h>
#include <lib/support/TestPersistentStorageDelegate.h>
#include <unistd.h>

namespace {

using namespace chip;
using namespace chip::app;
using namespace chip::Testing;

TEST(TestAttributePersistenceMigration, TestMigrationSuccess)
{
    TestPersistentStorageDelegate storageDelegate;
    DefaultAttributePersistenceProvider ramProvider;
    DefaultSafeAttributePersistenceProvider safeRamProvider;
    ASSERT_EQ(ramProvider.Init(&storageDelegate), CHIP_NO_ERROR);
    ASSERT_EQ(safeRamProvider.Init(&storageDelegate), CHIP_NO_ERROR);

    const ConcreteAttributePath path(1, 2, 3);
    constexpr uint32_t kValueToStore = 42;

    // Store a fake value
    {
        EXPECT_EQ(safeRamProvider.WriteScalarValue(path, kValueToStore), CHIP_NO_ERROR);
    }

}


}