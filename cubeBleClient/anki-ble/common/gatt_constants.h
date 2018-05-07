/**
 * File: gatt_constants.h
 *
 * Author: seichert
 * Created: 1/17/2018
 *
 * Description: Bluetooth GATT Constants for Anki Bluetooth Daemon
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

const uint16_t kCCCDefaultValue = 0x0000;
const uint16_t kCCCNotificationValue = 0x0001;
const uint16_t kCCCIndicationValue = 0x0002;

const int kGattPermRead = (1 << 0);
const int kGattPermReadEncrypted = (1 << 1);
const int kGattPermReadEncMitm = (1 << 2);
const int kGattPermWrite = (1 << 4);
const int kGattPermWriteEncrypted = (1 << 5);
const int kGattPermWriteEncMitm = (1 << 6);
const int kGattPermWriteSigned = (1 << 7);
const int kGattPermWriteSignedMitm = (1 << 8);

const int kGattCharacteristicPropBroadcast = (1 << 0);
const int kGattCharacteristicPropRead = (1 << 1);
const int kGattCharacteristicPropWriteNoResponse = (1 << 2);
const int kGattCharacteristicPropWrite = (1 << 3);
const int kGattCharacteristicPropNotify = (1 << 4);
const int kGattCharacteristicPropIndicate = (1 << 5);
const int kGattCharacteristicPropAuth = (1 << 6);
const int kGattCharacteristicPropExtPop = (1 << 7);

const int kGattStatusSuccess = 0;
const int kGattStatusFail = 1;
const int kGattStatusNotReady = 2;
const int kGattStatusNoMem = 3;
const int kGattStatusBusy = 4;
const int kGattStatusDone = 5;
const int kGattStatusUnsupported = 6;
const int kGattStatusParmInvalid = 7;
const int kGattStatusUnhandled = 8;
const int kGattStatusAuthFailure = 9;
const int kGattStatusRmtDevDown = 10;
const int kGattStatusAuthRejected = 11;
const int kGattStatusJniEnvironmentError = 12;
const int kGattStatusJniThreadAttachError = 13;
const int kGattStatusWakelockError = 14;

const int kGattErrorNone = 0;
const int kGattErrorInvalidHandle = 0x01;
const int kGattErrorReadNotPermitted = 0x02;
const int kGattErrorWriteNotPermitted = 0x03;
const int kGattErrorInvalidPDU = 0x04;
const int kGattErrorInsufficientAuthen = 0x05;
const int kGattErrorRequestNotSupported = 0x06;
const int kGattErrorInvalidOffset = 0x07;
const int kGattErrorInsufficientAuthor = 0x08;
const int kGattErrorPrepQueueFull = 0x09;
const int kGattErrorAttributeNotFound = 0x0a;
const int kGattErrorAttributeNotLong = 0x0b;
const int kGattErrorInsufficientKeySize = 0x0c;
const int kGattErrorInvalidAttributeLength = 0x0d;
const int kGattErrorUnlikely = 0x0e;
const int kGattErrorInsufficientEncr = 0x0f;
const int kGattErrorUnsupportedGrpType = 0x10;
const int kGattErrorInsufficientResources = 0x11;
const int kGattConnTerminateLocalHost = 0x16;
const int kGattErrorIllegalParameter = 0x87;
const int kGattErrorNoResources = 0x80;
const int kGattErrorInternalError = 0x81;
const int kGattErrorWrongState = 0x82;
const int kGattErrorDbFull = 0x83;
const int kGattErrorBusy = 0x84;
const int kGattErrorError = 0x85;
const int kGattErrorCmdStarted = 0x86;
const int kGattErrorPending = 0x88;
const int kGattErrorAuthFail = 0x89;
const int kGattErrorMore = 0x8a;
const int kGattErrorInvalidConfig = 0x8b;
const int kGattErrorServiceStarted = 0x8c;
const int kGattErrorEncryptedMITM = kGattErrorNone;
const int kGattErrorEncryptedNoMITM = 0x8d;
const int kGattErrorNotEncrypted = 0x8e;
const int kGattErrorCongested = 0x8f;
const int kGattErrorCCCDImproperlyConfigured = 0xfd;
const int kGattErrorProcedureInProgress = 0xfe;
const int kGattErrorOutOfRange = 0xff;

const uint8_t kADTypeFlags = 0x01;
const uint8_t kADTypeCompleteListOf16bitServiceClassUUIDs = 0x03;
const uint8_t kADTypeCompleteListOf128bitServiceClassUUIDs = 0x07;
const uint8_t kADTypeCompleteLocalName = 0x09;
const uint8_t kADTypeManufacturerSpecificData = 0xff;

const uint8_t kGattWriteTypeNoResponse = 1;
const uint8_t kGattWriteTypeWithResponse = 2;
const uint8_t kGattWriteTypePrepare = 3;

const int kGattConnectionIntervalMinimumDefault = 24; /* 30ms = 24 * 1.25ms */
const int kGattConnectionIntervalMaximumDefault = 40; /* 50ms = 40 * 1.25ms */
const int kGattConnectionLatencyDefault = 0;
const int kGattConnectionTimeoutDefault = 2000;

const int kGattConnectionIntervalHighPriorityMinimum = 9; /* 11.25ms = 9 * 1.25ms */
const int kGattConnectionIntervalHighPriorityMaximum = 12; /* 15ms = 12 * 1.25ms */
