#include <app/persistence/AttributePersistenceMigration.h>
namespace chip::app {

CHIP_ERROR MigrateFromSafeAttributePersistenceProvider(SafeAttributePersistenceProvider & safeProvider,
                                                       AttributePersistenceProvider & normProvider,
                                                       const ConcreteClusterPath & cluster,
                                                       Span<const std::pair<const AttributeId, SafeAttributeMigrator>> attributes,
                                                       MutableByteSpan & buffer)
{
    ChipError err = CHIP_NO_ERROR;

    ConcreteAttributePath attrPath;

    for (const auto & [attr, migrator] : attributes)
    {
        // We make a copy of the buffer so it can be resized
        // Still refers to same internal buffer though
        MutableByteSpan copyOfBuffer = buffer;
        attrPath                     = ConcreteAttributePath(cluster.mEndpointId, cluster.mClusterId, attr);

        // Read value from the safe provider, will resize copyOfBuffer to read size
        err = migrator(attrPath, safeProvider, copyOfBuffer);
        if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
        {
            // Attribute not in safe provider, nothing to migrate
            continue;
        }

        // Always delete from the safe provider to ensure we only attempt migration once.
        // This avoids overwriting a newer runtime value with a stale persisted one on the next startup.
        RETURN_SAFELY_IGNORED safeProvider.SafeDeleteValue(attrPath);

        if (err != CHIP_NO_ERROR)
        {
            continue;
        }

        err = normProvider.WriteValue(attrPath, copyOfBuffer);
    }
    return err;
};
} // namespace chip::app
