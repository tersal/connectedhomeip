#include <app/clusters/boolean-state-configuration-server/MigrateBooleanStateConfigurationServerStorage.h>
#include <app/clusters/boolean-state-configuration-server/BooleanStateConfigurationCluster.h>

namespace chip {
namespace app {
namespace Clusters {
namespace BooleanStateConfiguration {

CHIP_ERROR MigrateBooleanStateConfigurationServerStorage(EndpointId endpointId, SafeAttributePersistenceProvider & safeProvider,
                                                AttributePersistenceProvider & dstProvider)
{
    static constexpr AttrMigrationData attributesToUpdate[] = { { Attributes::CurrentSensitivityLevel::Id,
                                                                  &DefaultMigrators::ScalarValue<uint8_t> } };
    // We need to provide a buffer with enough space for the attributes that will be migrated.
    uint8_t attributeBuffer[sizeof(uint8_t)] = {};
    MutableByteSpan buffer(attributeBuffer);
    return MigrateFromSafeToAttributePersistenceProvider(safeProvider, dstProvider, { endpointId, BooleanStateConfiguration::Id },
                                                         Span(attributesToUpdate), buffer);
}

} // namespace BooleanStateConfiguration
} // namespace Clusters
} // namespace app
} // namespace chip