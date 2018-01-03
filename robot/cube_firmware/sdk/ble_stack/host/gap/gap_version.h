/**
 ****************************************************************************************
 *
 * @file gap_version.h
 *
 * @brief Definition of BLE Host version macros.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef GAP_VERSION_H_
#define GAP_VERSION_H_

// Version has the form Major.minor.release
/// Major version. This value is coming from the assigned numbers and corresponds to the
/// version of specification that the host is following.
/// For more information, go to https://www.bluetooth.org/Technical/AssignedNumbers/link_manager.htm
#define BLE_HOST_VERSION_MAJ       6  // From Assigned Numbers
#ifndef __DA14581__
/// Minor version. This value is incremented when new features are added to the host
#define BLE_HOST_VERSION_MIN       7

/// Release number. This value is incremented for every delivery done to the validation
/// team prior to release to customer
#define BLE_HOST_VERSION_REL       8
#else
/// Minor version. This value is incremented when new features are added to the host
#define BLE_HOST_VERSION_MIN       8

/// Release number. This value is incremented for every delivery done to the validation
/// team prior to release to customer
#define BLE_HOST_VERSION_REL      10
#endif

#endif // GAP_VERSION_H_
