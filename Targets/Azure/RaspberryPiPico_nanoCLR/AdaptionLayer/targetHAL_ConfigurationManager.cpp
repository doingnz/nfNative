//
// Copyright (c) .NET Foundation and Contributors
// See LICENSE file in the project root for full license information.
//

#include <nanoHAL.h>
#include <nanoHAL_v2.h>
#include <nanoWeak.h>
#include "FlashDriver.h"

uint32_t GetExistingConfigSize() {
  uint32_t currentConfigSize = 0;

  currentConfigSize = g_TargetConfiguration.NetworkInterfaceConfigs->Count *
                      sizeof(HAL_Configuration_NetworkInterface);
  currentConfigSize += g_TargetConfiguration.Wireless80211Configs->Count *
                       sizeof(HAL_Configuration_Wireless80211);

  return currentConfigSize;
}

// initialization of configuration manager
// provided as weak so it can be replaced at target level, if required because
// of the target implementing the storage with a mechanism other then saving to
// flash
__nfweak void ConfigurationManager_Initialize() {
  // init g_TargetConfiguration
  memset(&g_TargetConfiguration, 0, sizeof(HAL_TARGET_CONFIGURATION));

  // enumerate the blocks
  ConfigurationManager_EnumerateConfigurationBlocks();
};

// Enumerates the configuration blocks from the configuration flash sector
// it's implemented with 'weak' attribute so it can be replaced at target level
// if a different persistance mechanism is used
__nfweak void ConfigurationManager_EnumerateConfigurationBlocks() {
  // start checking if this device has config block
  if (((uint32_t)&__nanoConfig_end__ - (uint32_t)&__nanoConfig_start__) > 0) {
    // find network configuration blocks
    HAL_CONFIGURATION_NETWORK *networkConfigs = (HAL_CONFIGURATION_NETWORK *)
        ConfigurationManager_FindNetworkConfigurationBlocks(
            (uint32_t)&__nanoConfig_start__, (uint32_t)&__nanoConfig_end__);

    // check network configs count
    if (networkConfigs->Count == 0) {
      // there is no network config block available, get a default
      HAL_Configuration_NetworkInterface *networkConfig =
          (HAL_Configuration_NetworkInterface *)platform_malloc(
              sizeof(HAL_Configuration_NetworkInterface));

      if (InitialiseNetworkDefaultConfig(networkConfig, 0)) {
        // config block created, store it
        ConfigurationManager_StoreConfigurationBlock(
            networkConfig, DeviceConfigurationOption_Network, 0,
            sizeof(HAL_Configuration_NetworkInterface), 0, false);

        // have to enumerate again to pick it up
        networkConfigs = (HAL_CONFIGURATION_NETWORK *)
            ConfigurationManager_FindNetworkConfigurationBlocks(
                (uint32_t)&__nanoConfig_start__, (uint32_t)&__nanoConfig_end__);
      }

      platform_free(networkConfig);
    }

    // find wireless 80211 network configuration blocks
    HAL_CONFIGURATION_NETWORK_WIRELESS80211 *networkWirelessConfigs =
        (HAL_CONFIGURATION_NETWORK_WIRELESS80211 *)
            ConfigurationManager_FindNetworkWireless80211ConfigurationBlocks(
                (uint32_t)&__nanoConfig_start__, (uint32_t)&__nanoConfig_end__);

    // find X509 CA certificate blocks
    HAL_CONFIGURATION_X509_CERTIFICATE *certificateStore =
        (HAL_CONFIGURATION_X509_CERTIFICATE *)
            ConfigurationManager_FindX509CertificateConfigurationBlocks(
                (uint32_t)&__nanoConfig_start__, (uint32_t)&__nanoConfig_end__);

    // find X509 device certificate blocks
    HAL_CONFIGURATION_X509_DEVICE_CERTIFICATE *deviceCertificates =
        (HAL_CONFIGURATION_X509_DEVICE_CERTIFICATE *)
            ConfigurationManager_FindX509DeviceCertificatesConfigurationBlocks(
                (uint32_t)&__nanoConfig_start__, (uint32_t)&__nanoConfig_end__);

    // alloc memory for g_TargetConfiguration
    // because this is a struct of structs that use flexible members the memory
    // has to be allocated from the heap the malloc size for each struct is
    // computed separately
    uint32_t sizeOfNetworkInterfaceConfigs =
        offsetof(HAL_CONFIGURATION_NETWORK, Configs) +
        networkConfigs->Count * sizeof(networkConfigs->Configs[0]);
    uint32_t sizeOfWireless80211Configs =
        offsetof(HAL_CONFIGURATION_NETWORK_WIRELESS80211, Configs) +
        networkWirelessConfigs->Count *
            sizeof(networkWirelessConfigs->Configs[0]);
    uint32_t sizeOfX509CertificateStore =
        offsetof(HAL_CONFIGURATION_X509_CERTIFICATE, Certificates) +
        certificateStore->Count * sizeof(certificateStore->Certificates[0]);
    uint32_t sizeOfX509DeviceCertificate =
        offsetof(HAL_CONFIGURATION_X509_DEVICE_CERTIFICATE, Certificates) +
        deviceCertificates->Count * sizeof(deviceCertificates->Certificates[0]);

    g_TargetConfiguration.NetworkInterfaceConfigs =
        (HAL_CONFIGURATION_NETWORK *)platform_malloc(
            sizeOfNetworkInterfaceConfigs);
    g_TargetConfiguration.Wireless80211Configs =
        (HAL_CONFIGURATION_NETWORK_WIRELESS80211 *)platform_malloc(
            sizeOfWireless80211Configs);
    g_TargetConfiguration.CertificateStore =
        (HAL_CONFIGURATION_X509_CERTIFICATE *)platform_malloc(
            sizeOfX509CertificateStore);
    g_TargetConfiguration.DeviceCertificates =
        (HAL_CONFIGURATION_X509_DEVICE_CERTIFICATE *)platform_malloc(
            sizeOfX509DeviceCertificate);

    // copy structs to g_TargetConfiguration
    memcpy((HAL_CONFIGURATION_NETWORK *)
               g_TargetConfiguration.NetworkInterfaceConfigs,
           networkConfigs, sizeOfNetworkInterfaceConfigs);
    memcpy((HAL_CONFIGURATION_NETWORK_WIRELESS80211 *)
               g_TargetConfiguration.Wireless80211Configs,
           networkWirelessConfigs, sizeOfWireless80211Configs);
    memcpy((HAL_CONFIGURATION_X509_CERTIFICATE *)
               g_TargetConfiguration.CertificateStore,
           certificateStore, sizeOfX509CertificateStore);
    memcpy((HAL_CONFIGURATION_X509_DEVICE_CERTIFICATE *)
               g_TargetConfiguration.DeviceCertificates,
           deviceCertificates, sizeOfX509DeviceCertificate);

    // now free the memory of the original structs
    platform_free(networkConfigs);
    platform_free(networkWirelessConfigs);
    platform_free(certificateStore);
    platform_free(deviceCertificates);
  } else {
    // no config block
  }
}

// Gets the network configuration block from the configuration flash sector
// it's implemented with 'weak' attribute so it can be replaced at target level
// if a different persistance mechanism is used
__nfweak bool ConfigurationManager_GetConfigurationBlock(
    void *configurationBlock, DeviceConfigurationOption configuration,
    uint32_t configurationIndex) {
  int sizeOfBlock = 0;
  uint8_t *blockAddress = NULL;

  // validate if the requested block exists
  // Count has to be non zero
  // requested Index has to exist (array index starts at zero, so need to add
  // one)
  if (configuration == DeviceConfigurationOption_Network) {
    if (g_TargetConfiguration.NetworkInterfaceConfigs->Count == 0 ||
        (configurationIndex + 1) >
            g_TargetConfiguration.NetworkInterfaceConfigs->Count) {
      // the requested config block is beyond the available count
      return false;
    }

    // set block size
    sizeOfBlock = sizeof(HAL_Configuration_NetworkInterface);

    // get block address
    blockAddress = (uint8_t *)g_TargetConfiguration.NetworkInterfaceConfigs
                       ->Configs[configurationIndex];
  } else if (configuration == DeviceConfigurationOption_Wireless80211Network) {
    if (g_TargetConfiguration.Wireless80211Configs->Count == 0 ||
        (configurationIndex + 1) >
            g_TargetConfiguration.Wireless80211Configs->Count) {
      return FALSE;
    }

    // set block size
    sizeOfBlock = sizeof(HAL_Configuration_Wireless80211);

    // get block address
    blockAddress = (uint8_t *)g_TargetConfiguration.Wireless80211Configs
                       ->Configs[configurationIndex];
  } else if (configuration == DeviceConfigurationOption_X509CaRootBundle) {
    if (g_TargetConfiguration.CertificateStore->Count == 0 ||
        (configurationIndex + 1) >
            g_TargetConfiguration.CertificateStore->Count) {
      return FALSE;
    }

    // get block address
    blockAddress = (uint8_t *)g_TargetConfiguration.CertificateStore
                       ->Certificates[configurationIndex];

    // set block size
    // because X509 certificate has a variable length need to compute the block
    // size in two steps
    sizeOfBlock = offsetof(HAL_Configuration_X509CaRootBundle, Certificate);
    sizeOfBlock +=
        ((HAL_Configuration_X509CaRootBundle *)blockAddress)->CertificateSize;
  } else if (configuration == DeviceConfigurationOption_X509CaRootBundle) {
    if (g_TargetConfiguration.DeviceCertificates->Count == 0 ||
        (configurationIndex + 1) >
            g_TargetConfiguration.DeviceCertificates->Count) {
      return FALSE;
    }

    // get block address
    blockAddress = (uint8_t *)g_TargetConfiguration.DeviceCertificates
                       ->Certificates[configurationIndex];

    // set block size
    // because X509 certificate has a variable length need to compute the block
    // size in two steps
    sizeOfBlock =
        offsetof(HAL_Configuration_X509DeviceCertificate, Certificate);
    sizeOfBlock += ((HAL_Configuration_X509DeviceCertificate *)blockAddress)
                       ->CertificateSize;
  }
  // copy the config block content to the pointer in the argument
  memcpy(configurationBlock, blockAddress, sizeOfBlock);

  return TRUE;
}

// Stores the configuration block to the configuration flash sector
// NOTE: because inserting or removing a configuration block it's very 'RAM
// expensive' we choose not to support those operations the host debugger will
// have to be used to manage these operations on the device configuration
// collection it's implemented with 'weak' attribute so it can be replaced at
// target level if a different persistance mechanism is used
__nfweak bool ConfigurationManager_StoreConfigurationBlock(
    void *configurationBlock, DeviceConfigurationOption configuration,
    uint32_t configurationIndex, uint32_t blockSize, uint32_t offset,
    bool done) {
  ByteAddress storageAddress = 0;
  bool requiresEnumeration = FALSE;
  bool success = FALSE;

  if (configuration == DeviceConfigurationOption_Network) {
    if (g_TargetConfiguration.NetworkInterfaceConfigs == NULL ||
        (g_TargetConfiguration.NetworkInterfaceConfigs->Count == 0 &&
         configurationIndex == 0)) {
      // there is no network config block, we are storing the default one
      // THIS IS THE ONLY CONFIG BLOCK THAT'S AUTO-CREATED
      // OK to continue
      // set storage address as the start of the flash configuration sector
      storageAddress = (ByteAddress)&__nanoConfig_start__;
    } else {
      // the requested config block is beyond the available count
      if ((configurationIndex + 1) >
          g_TargetConfiguration.NetworkInterfaceConfigs->Count) {
        return FALSE;
      }

      // set storage address from block address, plus the requested offset
      storageAddress =
          (ByteAddress)g_TargetConfiguration.NetworkInterfaceConfigs
              ->Configs[configurationIndex] +
          offset;
    }

    // set block size, in case it's not already set
    blockSize = sizeof(HAL_Configuration_NetworkInterface);

    // make sure the config block marker is set
    memcpy(configurationBlock, c_MARKER_CONFIGURATION_NETWORK_V1,
           sizeof(c_MARKER_CONFIGURATION_NETWORK_V1));

    _ASSERTE(((HAL_Configuration_NetworkInterface *)configurationBlock)
                 ->StartupAddressMode > 0);
  } else if (configuration == DeviceConfigurationOption_Wireless80211Network) {
    if (g_TargetConfiguration.Wireless80211Configs == NULL ||
        (g_TargetConfiguration.Wireless80211Configs->Count == 0 ||
         (configurationIndex + 1) >
             g_TargetConfiguration.Wireless80211Configs->Count)) {
      // there is no room for this block, or there are no blocks stored at all
      // failing the operation
      return FALSE;
    }

    // set storage address from block address, plus the requested offset
    storageAddress = (ByteAddress)g_TargetConfiguration.Wireless80211Configs
                         ->Configs[configurationIndex] +
                     offset;

    // set block size, in case it's not already set
    blockSize = sizeof(HAL_Configuration_Wireless80211);

    // make sure the config block marker is set
    memcpy(configurationBlock, c_MARKER_CONFIGURATION_WIRELESS80211_V1,
           sizeof(c_MARKER_CONFIGURATION_WIRELESS80211_V1));
  } else if (configuration == DeviceConfigurationOption_X509CaRootBundle) {
    // compute block size
    // because X509 certificate has a variable length need to compute the block
    // size in two steps
    blockSize = offsetof(HAL_Configuration_X509CaRootBundle, Certificate);
    blockSize += ((HAL_Configuration_X509CaRootBundle *)configurationBlock)
                     ->CertificateSize;

    //
    if (g_TargetConfiguration.CertificateStore->Count == 0) {
      // there is nothing at the certificate store
      // find size of existing config blocks
      storageAddress =
          (uint32_t)&__nanoConfig_start__ + GetExistingConfigSize();
    } else {
      // set storage address from block address, plus the requested offset
      storageAddress = (ByteAddress)g_TargetConfiguration.CertificateStore
                           ->Certificates[configurationIndex] +
                       offset;
    }

    if (g_TargetConfiguration.CertificateStore == NULL ||
        (g_TargetConfiguration.CertificateStore->Count == 0 ||
         (configurationIndex + 1) >
             g_TargetConfiguration.CertificateStore->Count)) {
      // there is no block stored
      // check if there is room for it
      if (((uint32_t)&__nanoConfig_end__ - storageAddress) < blockSize) {
        // not enough room
        return FALSE;
      }

      // now check if memory is erase, so the block can be stored
      if (!FlashDriver_IsBlockErased(NULL, storageAddress, blockSize)) {
        // memory not erased, can't store
        return FALSE;
      }
    }

    // make sure the config block marker is set, ONLY required on the 1st chunk
    if (offset == 0) {
      memcpy(configurationBlock, c_MARKER_CONFIGURATION_X509CAROOTBUNDLE_V1,
             sizeof(c_MARKER_CONFIGURATION_X509CAROOTBUNDLE_V1));
    }
  } else if (configuration ==
             DeviceConfigurationOption_X509DeviceCertificates) {
    // compute block size
    // because X509 certificate has a variable length need to compute the block
    // size in two steps
    blockSize = offsetof(HAL_Configuration_X509DeviceCertificate, Certificate);
    blockSize += ((HAL_Configuration_X509DeviceCertificate *)configurationBlock)
                     ->CertificateSize;

    //
    if (g_TargetConfiguration.DeviceCertificates->Count == 0) {
      // there is nothing at the certificate store
      // find size of existing config blocks
      storageAddress =
          (uint32_t)&__nanoConfig_start__ + GetExistingConfigSize();
    } else {
      // set storage address from block address, plus the requested offset
      storageAddress = (ByteAddress)g_TargetConfiguration.DeviceCertificates
                           ->Certificates[configurationIndex] +
                       offset;
    }

    if (g_TargetConfiguration.DeviceCertificates == NULL ||
        (g_TargetConfiguration.DeviceCertificates->Count == 0 ||
         (configurationIndex + 1) >
             g_TargetConfiguration.DeviceCertificates->Count)) {
      // there is no block stored
      // check if there is room for it
      if (((uint32_t)&__nanoConfig_end__ - storageAddress) < blockSize) {
        // not enough room
        return FALSE;
      }

      // now check if memory is erase, so the block can be stored
      if (!FlashDriver_IsBlockErased(NULL, storageAddress, blockSize)) {
        // memory not erased, can't store
        return FALSE;
      }
    }

    // make sure the config block marker is set, ONLY required on the 1st chunk
    if (offset == 0) {
      memcpy(configurationBlock,
             c_MARKER_CONFIGURATION_X509DEVICECERTIFICATE_V1,
             sizeof(c_MARKER_CONFIGURATION_X509DEVICECERTIFICATE_V1));
    }
  } else if (configuration == DeviceConfigurationOption_All) {
    // particular situation where we are receiving the full configuration block

    // set storage address as the start of the flash configuration sector, plus
    // the requested offset
    storageAddress = (ByteAddress)&__nanoConfig_start__ + offset;

    // for save all the block size has to be provided, check that
    if (blockSize == 0) {
      return FALSE;
    }
  }

  // copy the config block content to the config block storage
  success = FlashDriver_Write(NULL, storageAddress, blockSize,
                              (unsigned char *)configurationBlock, true);

  // enumeration is required after we are DONE with SUCCESSFULLY storing all the
  // config chunks
  requiresEnumeration = (success && done);

  if (requiresEnumeration) {
    // free the current allocation(s)
    platform_free(g_TargetConfiguration.NetworkInterfaceConfigs);
    platform_free(g_TargetConfiguration.Wireless80211Configs);
    platform_free(g_TargetConfiguration.CertificateStore);
    platform_free(g_TargetConfiguration.DeviceCertificates);

    // perform enumeration of configuration blocks
    ConfigurationManager_EnumerateConfigurationBlocks();
  }

  return success;
}

// Updates a configuration block in the configuration flash sector
// The flash sector has to be erased before writing the updated block
// it's implemented with 'weak' attribute so it can be replaced at target level
// if a different persistance mechanism is used
__nfweak bool ConfigurationManager_UpdateConfigurationBlock(
    void *configurationBlock, DeviceConfigurationOption configuration,
    uint32_t configurationIndex) {
  ByteAddress storageAddress;
  uint32_t blockOffset;
  uint8_t *blockAddressInCopy;
  uint32_t blockSize;
  bool success = FALSE;

  // config sector size
  int sizeOfConfigSector =
      (uint32_t)&__nanoConfig_end__ - (uint32_t)&__nanoConfig_start__;

  // allocate memory from CRT heap
  uint8_t *configSectorCopy = (uint8_t *)platform_malloc(sizeOfConfigSector);

  if (configSectorCopy != NULL) {
    // copy config sector from flash to RAM
    memcpy(configSectorCopy, &__nanoConfig_start__, sizeOfConfigSector);

    // find out the address for the config block to update in the
    // configSectorCopy because we are copying back the config block to flash
    // and just replacing the config block content the addresses in
    // g_TargetConfiguration will remain the same plus we can calculate the
    // offset of the config block from g_TargetConfiguration
    if (configuration == DeviceConfigurationOption_Network) {
      // make sure the config block marker is set
      memcpy(configurationBlock, c_MARKER_CONFIGURATION_NETWORK_V1,
             sizeof(c_MARKER_CONFIGURATION_NETWORK_V1));

      // check if the configuration block is the same
      if (ConfigurationManager_CheckExistingConfigurationBlock(
              g_TargetConfiguration.NetworkInterfaceConfigs
                  ->Configs[configurationIndex],
              configurationBlock, sizeof(HAL_Configuration_NetworkInterface),
              sizeof(HAL_Configuration_NetworkInterface))) {
        // block is the same
        // free memory
        platform_free(configSectorCopy);

        // operation is successfull (nothing to update)
        return TRUE;
      }

      // get storage address from block address
      storageAddress =
          (ByteAddress)g_TargetConfiguration.NetworkInterfaceConfigs
              ->Configs[configurationIndex];

      // set block size, in case it's not already set
      blockSize = sizeof(HAL_Configuration_NetworkInterface);

      _ASSERTE(((HAL_Configuration_NetworkInterface *)configurationBlock)
                   ->StartupAddressMode > 0);
    } else if (configuration ==
               DeviceConfigurationOption_Wireless80211Network) {
      // make sure the config block marker is set
      memcpy(configurationBlock, c_MARKER_CONFIGURATION_WIRELESS80211_V1,
             sizeof(c_MARKER_CONFIGURATION_WIRELESS80211_V1));

      // check if the configuration block is the same
      if (ConfigurationManager_CheckExistingConfigurationBlock(
              g_TargetConfiguration.Wireless80211Configs
                  ->Configs[configurationIndex],
              configurationBlock, sizeof(HAL_Configuration_Wireless80211),
              sizeof(HAL_Configuration_Wireless80211))) {
        // block is the same
        // free memory
        platform_free(configSectorCopy);

        // operation is successfull (nothing to update)
        return TRUE;
      }

      // storage address from block address
      storageAddress = (ByteAddress)g_TargetConfiguration.Wireless80211Configs
                           ->Configs[configurationIndex];

      // set block size, in case it's not already set
      blockSize = sizeof(HAL_Configuration_Wireless80211);
    } else if (configuration == DeviceConfigurationOption_X509CaRootBundle) {
      // make sure the config block marker is set
      memcpy(configurationBlock, c_MARKER_CONFIGURATION_X509CAROOTBUNDLE_V1,
             sizeof(c_MARKER_CONFIGURATION_X509CAROOTBUNDLE_V1));

      // check if certificate is the same
      if (ConfigurationManager_CheckExistingConfigurationBlock(
              g_TargetConfiguration.CertificateStore
                  ->Certificates[configurationIndex]
                  ->Certificate,
              ((HAL_Configuration_X509CaRootBundle *)configurationBlock)
                  ->Certificate,
              g_TargetConfiguration.CertificateStore
                  ->Certificates[configurationIndex]
                  ->CertificateSize,
              ((HAL_Configuration_X509CaRootBundle *)configurationBlock)
                  ->CertificateSize)) {
        // block is the same
        // free memory
        platform_free(configSectorCopy);

        // operation is successfull (nothing to update)
        return TRUE;
      }

      // storage address from block address
      storageAddress = (ByteAddress)g_TargetConfiguration.CertificateStore
                           ->Certificates[configurationIndex];

      // set block size, in case it's not already set
      // because X509 certificate has a variable length need to compute the
      // block size in two steps
      blockSize = offsetof(HAL_Configuration_X509CaRootBundle, Certificate);
      blockSize += ((HAL_Configuration_X509CaRootBundle *)configurationBlock)
                       ->CertificateSize;
    } else if (configuration ==
               DeviceConfigurationOption_X509DeviceCertificates) {
      // make sure the config block marker is set
      memcpy(configurationBlock,
             c_MARKER_CONFIGURATION_X509DEVICECERTIFICATE_V1,
             sizeof(c_MARKER_CONFIGURATION_X509DEVICECERTIFICATE_V1));

      // check if certificate is the same
      if (ConfigurationManager_CheckExistingConfigurationBlock(
              g_TargetConfiguration.DeviceCertificates
                  ->Certificates[configurationIndex]
                  ->Certificate,
              ((HAL_Configuration_X509DeviceCertificate *)configurationBlock)
                  ->Certificate,
              g_TargetConfiguration.DeviceCertificates
                  ->Certificates[configurationIndex]
                  ->CertificateSize,
              ((HAL_Configuration_X509DeviceCertificate *)configurationBlock)
                  ->CertificateSize)) {
        // block is the same
        // free memory
        platform_free(configSectorCopy);

        // operation is successfull (nothing to update)
        return TRUE;
      }

      // storage address from block address
      storageAddress = (ByteAddress)g_TargetConfiguration.DeviceCertificates
                           ->Certificates[configurationIndex];

      // set block size, in case it's not already set
      // because X509 certificate has a variable length need to compute the
      // block size in two steps
      blockSize =
          offsetof(HAL_Configuration_X509DeviceCertificate, Certificate);
      blockSize +=
          ((HAL_Configuration_X509DeviceCertificate *)configurationBlock)
              ->CertificateSize;
    } else {
      // this not a valid configuration option to update, quit
      // free memory first
      platform_free(configSectorCopy);

      return FALSE;
    }

    // erase config sector
    if (FlashDriver_EraseBlock(NULL, (uint32_t)&__nanoConfig_start__) == TRUE) {
      // flash block is erased

      // subtract the start address of config sector to get the offset
      blockOffset = storageAddress - (uint32_t)&__nanoConfig_start__;

      // set pointer to block to udpate
      blockAddressInCopy = configSectorCopy + blockOffset;

      // replace config block with new content by replacing memory
      memcpy(blockAddressInCopy, configurationBlock, blockSize);

      // copy the config block copy back to the config block storage
      success = FlashDriver_Write(NULL, (uint32_t)&__nanoConfig_start__,
                                  sizeOfConfigSector,
                                  (unsigned char *)configSectorCopy, true);
    }

    // free memory
    platform_free(configSectorCopy);
  }

  return success;
}

//  Default initialisation for wireless config block
// it's implemented with 'weak' attribute so it can be replaced at target level
// if different configurations are intended
__nfweak void
InitialiseWirelessDefaultConfig(HAL_Configuration_Wireless80211 *pconfig,
                                uint32_t configurationIndex) {
  (void)pconfig;
  (void)configurationIndex;

  // currently empty as no ChibiOS target has Wireless 802.11 interface
}

//  Default initialisation for Network interface config blocks
// it's implemented with 'weak' attribute so it can be replaced at target level
// if different configurations are intended
__nfweak bool
InitialiseNetworkDefaultConfig(HAL_Configuration_NetworkInterface *pconfig,
                               uint32_t configurationIndex) {
  (void)pconfig;
  (void)configurationIndex;

  // can't create a "default" network config because we are lacking definition
  // of a MAC address
  return FALSE;
}

#ifdef NewVersion

#define VALID_PERSISTENT_STORAGE_CONFIG 44 // Random chosen constant
#define CONFIG_BASE __nanoConfig_start__

typedef struct __nfpack {
  uint8_t ValidConfiguration;
  HAL_Configuration_NetworkInterface *Data;
} NetworkInterfaceDataLocation;

typedef struct __nfpack {
  uint8_t ValidConfiguration;
  HAL_Configuration_Wireless80211 *Data;
} Wireless80211DataLocation;

typedef struct __nfpack {
  uint8_t ValidConfiguration;
  HAL_Configuration_WirelessAP *Data;
} WirelessAPDataLocation;

typedef struct __nfpack {
  uint8_t ValidConfiguration;
  HAL_Configuration_X509DeviceCertificate *Data;
} X509DeviceCertificateDataLocation;

typedef struct __nfpack {
  uint8_t ValidConfiguration;
  HAL_Configuration_X509CaRootBundle *Data;
} X509CaRootBundleDataLocation;

#define PERSISTENCE_HEADER CONFIG_BASE
#define NetworkInterfaceConfig PERSISTENCE_HEADER + sizeof(PersistenceHeader)
#define Wireless80211Config                                                    \
  (NetworkInterfaceConfig + sizeof(NetworkInterfaceDataLocation))
#define WirelessAPConfig                                                       \
  (Wireless80211Config + sizeof(Wireless80211DataLocation))
#define X509DeviceCertificateConfig                                            \
  (X509RootBundleConfig + sizeof(WirelessAPDataLocation))
#define X509RootBundleConfig                                                   \
  (WirelessAPConfig + sizeof(X509DeviceCertificateDataLocation))

typedef struct __nfpack {
  NetworkInterfaceDataLocation
      NetworkInterface;                    // HAL_Configuration_NetworkInterface
  Wireless80211DataLocation Wireless80211; // HAL_Configuration_Wireless80211
  WirelessAPDataLocation WirelessAP;       // HAL_Configuration_WirelessAP
  X509CaRootBundleDataLocation
      X509RootBundle; // HAL_Configuration_X509CaRootBundle
  X509DeviceCertificateDataLocation
      X509DeviceCertificate; // HAL_Configuration_X509DeviceCertificate
} PersistenceHeader;

PersistenceHeader ph;

uint32_t GetExistingConfigSize() { return 0; }

void ConfigurationManager_Initialize(){};

void ConfigurationManager_EnumerateConfigurationBlocks() {
  // No Enumeration
}

bool ConfigurationManager_GetConfigurationBlock(
    void *configurationBlock, DeviceConfigurationOption configuration,
    uint32_t configurationIndex) {
  bool status = false; // False, unless success

  // Read the Persistence Header information
  EmbeddedFlashReadBytes(CONFIG_BASE, sizeof(PersistenceHeader),
                         (uint8_t *)&ph);

  switch (configuration) {
  case DeviceConfigurationOption_Network:
    if (ph.NetworkInterface.ValidConfiguration) {
      EmbeddedFlashReadBytes((uint32_t)ph.NetworkInterface.Data,
                             sizeof(HAL_Configuration_NetworkInterface),
                             (uint8_t *)configurationBlock);
      status = true;
    }
    break;
  case DeviceConfigurationOption_Wireless80211Network:
    if (ph.Wireless80211.ValidConfiguration) {
      EmbeddedFlashReadBytes((uint32_t)ph.Wireless80211.Data,
                             sizeof(HAL_Configuration_Wireless80211),
                             (uint8_t *)configurationBlock);
      status = true;
    }
    break;
  case DeviceConfigurationOption_WirelessNetworkAP:
    if (ph.WirelessAP.ValidConfiguration) {
      EmbeddedFlashReadBytes((uint32_t)ph.WirelessAP.Data,
                             sizeof(HAL_Configuration_WirelessAP),
                             (uint8_t *)configurationBlock);
      status = true;
    }
    break;
  case DeviceConfigurationOption_X509CaRootBundle:
    if (ph.X509RootBundle.ValidConfiguration) {
      EmbeddedFlashReadBytes((uint32_t)ph.X509RootBundle.Data,
                             sizeof(HAL_Configuration_X509CaRootBundle),
                             (uint8_t *)configurationBlock);
      status = true;
    }
    break;
  case DeviceConfigurationOption_X509DeviceCertificates:
    if (ph.X509DeviceCertificate.ValidConfiguration) {
      EmbeddedFlashReadBytes((uint32_t)ph.X509DeviceCertificate.Data,
                             sizeof(HAL_Configuration_X509DeviceCertificate),
                             (uint8_t *)configurationBlock);
      status = true;
    }
    break;
  case DeviceConfigurationOption_All:
    break;
  }
  return status;
}

bool ConfigurationManager_StoreConfigurationBlock(
    void *configurationBlock, DeviceConfigurationOption configuration,
    uint32_t configurationIndex, uint32_t blockSize, uint32_t offset,
    bool done) {
  bool status = false; // False, unless success

  // Read the Persistence Header information
  EmbeddedFlashReadBytes(PERSISTENCE_HEADER, sizeof(PersistenceHeader),
                         (uint8_t *)&ph);

  switch (configuration) {
  case DeviceConfigurationOption_Network:
    ph.NetworkInterface.ValidConfiguration = VALID_PERSISTENT_STORAGE_CONFIG;
    EmbeddedFlashWrite(PERSISTENCE_HEADER, sizeof(PersistenceHeader),
                       (uint8_t *)&ph);
    EmbeddedFlashWrite(NetworkInterfaceConfig,
                       sizeof(HAL_Configuration_NetworkInterface),
                       (uint8_t *)&ph);
    status = true;
    break;
  case DeviceConfigurationOption_Wireless80211Network:
    ph.Wireless80211.ValidConfiguration = VALID_PERSISTENT_STORAGE_CONFIG;
    EmbeddedFlashWrite(PERSISTENCE_HEADER, sizeof(PersistenceHeader),
                       (uint8_t *)&ph);
    EmbeddedFlashWrite(Wireless80211Config,
                       sizeof(HAL_Configuration_Wireless80211), (uint8_t *)&ph);
    status = true;
    break;
  case DeviceConfigurationOption_WirelessNetworkAP:
    ph.WirelessAP.ValidConfiguration = VALID_PERSISTENT_STORAGE_CONFIG;
    EmbeddedFlashWrite(WirelessAPConfig, sizeof(PersistenceHeader),
                       (uint8_t *)&ph);
    EmbeddedFlashWrite(WirelessAPConfig, sizeof(HAL_Configuration_WirelessAP),
                       (uint8_t *)&ph);
    status = true;
    break;
  case DeviceConfigurationOption_X509CaRootBundle:
    ph.X509RootBundle.ValidConfiguration = VALID_PERSISTENT_STORAGE_CONFIG;
    EmbeddedFlashWrite(PERSISTENCE_HEADER, sizeof(PersistenceHeader),
                       (uint8_t *)&ph);
    EmbeddedFlashWrite(X509RootBundleConfig,
                       sizeof(HAL_Configuration_X509CaRootBundle),
                       (uint8_t *)&ph);
    status = true;
    break;
  case DeviceConfigurationOption_X509DeviceCertificates:
    ph.X509DeviceCertificate.ValidConfiguration =
        VALID_PERSISTENT_STORAGE_CONFIG;
    EmbeddedFlashWrite(PERSISTENCE_HEADER, sizeof(PersistenceHeader),
                       (uint8_t *)&ph);
    EmbeddedFlashWrite(X509DeviceCertificateConfig,
                       sizeof(HAL_Configuration_X509DeviceCertificate),
                       (uint8_t *)&ph);
    status = true;
    break;
  case DeviceConfigurationOption_All:
    break;
  }
  return true;

  return true;
}
bool ConfigurationManager_UpdateConfigurationBlock(
    void *configurationBlock, DeviceConfigurationOption configuration,
    uint32_t configurationIndex) {
  switch (configuration) {
  case DeviceConfigurationOption_Network:
    break;
  case DeviceConfigurationOption_Wireless80211Network:
    break;
  case DeviceConfigurationOption_WirelessNetworkAP:
    break;
  case DeviceConfigurationOption_X509CaRootBundle:
    break;
  case DeviceConfigurationOption_X509DeviceCertificates:
    break;
  case DeviceConfigurationOption_All:
    break;
  }

  return true;
}
void InitialiseWirelessDefaultConfig(HAL_Configuration_Wireless80211 *pconfig,
                                     uint32_t configurationIndex) {
  (void)pconfig;
  (void)configurationIndex;
}
bool InitialiseNetworkDefaultConfig(HAL_Configuration_NetworkInterface *pconfig,
                                    uint32_t configurationIndex) {
  (void)pconfig;
  (void)configurationIndex;
  return FALSE;
}
#endif
