/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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
 *    @file
 *          Platform-specific configuration overrides for the CHIP BLE
 *          Layer on Darwin platforms.
 *
 */

#pragma once

// ==================== Platform Adaptations ====================

#define BLE_CONNECTION_OBJECT void * // actually __unsafe_unretained CBPeripheral *
#define BLE_CONNECTION_UNINITIALIZED nullptr
#define BLE_USES_DEVICE_EVENTS 0

// ========== Platform-specific Configuration Overrides =========

/* none so far */
