#include <app/persistence/AttributePersistenceMigration.h>
namespace chip::app {

CHIP_ERROR MigrateFromSafeAttributePersistenceProvider(SafeAttributePersistenceProvider & safeProvider,
                                                       AttributePersistenceProvider & normProvider,
                                                       const ConcreteClusterPath & cluster,
                                                       Span<const std::pair<const AttributeId, SafeAttributeMigrator>> attributes,
                                                       MutableByteSpan & buffer)
{
    ChipError err = CHIP_NO_ERROR;

    if (attributes.size() > 1)
    {
        // We make a copy of the buffer so it can be resized
        // Still refers to same internal buffer though
        MutableByteSpan copyOfBuffer = buffer;

        // Quick check to see if migration already happened
        auto firstAttributePath = ConcreteAttributePath(cluster.mEndpointId, cluster.mClusterId, attributes[0].first);
        err                     = normProvider.ReadValue(firstAttributePath, copyOfBuffer);
        // If the attribute is already in the standard Attribute provider it means that a migration already happened
        VerifyOrReturnError(err != CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND, CHIP_NO_ERROR);
    }

    ConcreteAttributePath attrPath;

    for (const auto & [attr, migrator] : attributes)
    {
        // We make a copy of the buffer so it can be resized
        // Still refers to same internal buffer though
        MutableByteSpan copyOfBuffer = buffer;
        attrPath                     = ConcreteAttributePath(cluster.mEndpointId, cluster.mClusterId, attr);

        // Read Value, will resize copyOfBuffer to read size
        err = migrator(attrPath, safeProvider, copyOfBuffer);
        if (err != CHIP_NO_ERROR)
        {
            continue;
        }

        err = normProvider.WriteValue(attrPath, copyOfBuffer);
        if (err != CHIP_NO_ERROR)
        {
            continue;
        }

        err = safeProvider.SafeDeleteValue(attrPath);
    }
    return err;
};
} // namespace chip::app
