/**
 *    Copyright (c) 2023-2025 Project CHIP Authors
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
#include <app/clusters/boolean-state-configuration-server/BooleanStateConfigurationCluster.h>
#include <app/clusters/boolean-state-configuration-server/MigrateBooleanStateConfigurationServerStorage.h>

namespace chip {
namespace app {
namespace Clusters {
namespace BooleanStateConfiguration {

CHIP_ERROR MigrateBooleanStateConfigurationServerStorage(EndpointId endpointId, SafeAttributePersistenceProvider & safeProvider,
                                                         AttributePersistenceProvider & dstProvider)
{
    static constexpr AttrMigrationData attributesToUpdate[] = { { Attributes::CurrentSensitivityLevel::Id,
                                                                  &DefaultMigrators::ScalarValue<uint8_t> } };

    // We need to provide a buffer with enough space for the largest of the attributes that will be migrated.
    using LargestAttributeType                            = uint8_t;
    uint8_t attributeBuffer[sizeof(LargestAttributeType)] = {};
    MutableByteSpan buffer(attributeBuffer);
    return MigrateFromSafeToAttributePersistenceProvider(safeProvider, dstProvider, { endpointId, BooleanStateConfiguration::Id },
                                                         Span(attributesToUpdate), buffer);
}

} // namespace BooleanStateConfiguration
} // namespace Clusters
} // namespace app
} // namespace chip
