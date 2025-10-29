#pragma once

#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-cluster.h>

namespace chip::app::Clusters::ValveConfigurationAndControl {

void SetDelegate(EndpointId endpointId,  DelegateBase * delegate);

} // namespace chip::app::Clusters::ValveConfigurationAndControl
