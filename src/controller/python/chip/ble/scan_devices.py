#
#    Copyright (c) 2021 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

import ctypes
from dataclasses import dataclass
from queue import Queue
from threading import Thread
from typing import Generator

from .library_handle import _GetBleLibraryHandle
from .types import DeviceScannedCallback, ScanDoneCallback, ScanErrorCallback


@DeviceScannedCallback
def ScanFoundCallback(closure, address: str, discriminator: int, vendor: int,
                      product: int):
    closure.OnDeviceScanned(address, discriminator, vendor, product)


@ScanDoneCallback
def ScanIsDoneCallback(closure):
    closure.OnScanComplete()


@ScanErrorCallback
def ScanHasErrorCallback(closure, errorCode: int):
    closure.OnScanError(errorCode)


@dataclass
class DeviceInfo:
    address: str
    discriminator: int
    vendor: int
    product: int


class _DeviceInfoReceiver:
    """Uses a queue to notify of objects received asynchronously
       from a BLE scan.

       Internal queue gets filled on DeviceFound and ends with None when
       ScanCompleted.
    """

    def __init__(self):
        self.queue = Queue()

    def OnDeviceScanned(self, address, discriminator, vendor, product):
        self.queue.put(DeviceInfo(address, discriminator, vendor, product))

    def OnScanComplete(self):
        self.queue.put(None)

    def OnScanError(self, errorCode):
        # TODO need to determine what we do with this error. Most of the time this
        # error is just a timeout introduced in PR #24873, right before we get a
        # ScanCompleted.
        pass


def DiscoverSync(timeoutMs: int, adapter=None) -> Generator[DeviceInfo, None, None]:
    """Discover BLE devices over the specified period of time.

    NOTE: Devices are not guaranteed to be unique. New entries are returned
    as soon as the underlying BLE manager detects changes.

    Args:
      timeoutMs:    scan will complete after this time
      adapter:      what adapter to choose. Either an AdapterInfo object or
                    a string with the adapter address. If None, the first
                    adapter on the system is used.
    """
    if adapter:
        if isinstance(adapter, str):
            adapter = adapter.upper()
        else:
            adapter = adapter.address

    handle = _GetBleLibraryHandle()

    nativeList = handle.pychip_ble_adapter_list_new()
    if nativeList == 0:
        raise Exception('Failed to list available adapters')

    try:
        while handle.pychip_ble_adapter_list_next(nativeList):
            if adapter and (adapter != handle.pychip_ble_adapter_list_get_address(
                    nativeList).decode('utf8')):
                continue

            receiver = _DeviceInfoReceiver()
            scanner = handle.pychip_ble_scanner_start(
                ctypes.py_object(receiver),
                handle.pychip_ble_adapter_list_get_raw_adapter(nativeList),
                timeoutMs, ScanFoundCallback, ScanIsDoneCallback, ScanHasErrorCallback)

            if scanner == 0:
                raise Exception('Failed to start BLE scan')

            while True:
                data = receiver.queue.get()
                if not data:
                    break
                yield data

            handle.pychip_ble_scanner_delete(scanner)
            break
    finally:
        handle.pychip_ble_adapter_list_delete(nativeList)


def DiscoverAsync(timeoutMs: int, scanCallback, doneCallback, errorCallback, adapter=None):
    """Discover BLE devices over the specified period of time without blocking.

    NOTE: Devices are not guaranteed to be unique. The scanCallback is called
    as soon as the underlying BLE manager detects changes.

    Args:
      timeoutMs:    scan will complete after this time
      scanCallback: callback when a device is found
      doneCallback: callback when the scan is complete
      errorCallback: callback when error occurred during scan
      adapter:      what adapter to choose. Either an AdapterInfo object or
                    a string with the adapter address. If None, the first
                    adapter on the system is used.
    """

    def _DiscoverAsync(timeoutMs, scanCallback, doneCallback, errorCallback, adapter):
        for device in DiscoverSync(timeoutMs, adapter):
            scanCallback(device.address, device.discriminator, device.vendor, device.product)
        doneCallback()

    t = Thread(target=_DiscoverAsync,
               args=(timeoutMs, scanCallback, doneCallback, errorCallback, adapter),
               daemon=True)
    t.start()
