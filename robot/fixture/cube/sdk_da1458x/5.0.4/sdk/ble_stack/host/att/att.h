/**
 ****************************************************************************************
 *
 * @file att.h
 *
 * @brief Header file - ATT.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATT_H_
#define ATT_H_

/**
 ****************************************************************************************
 * @addtogroup ATT Attribute Protocol
 * @ingroup HOST
 * @brief Attribute Protocol.
 *
 * The ATT block contains the procedures for discovering, reading, writing
 * and indicating attributes to peer device . It also defines a number of items
 * that caters to the security aspect of the block as access to some information
 * may require both authorization and an authenticated and encrypted physical
 * link before an attribute can be read or written
 *
 * @{
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "attm_cfg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Invalid attribute handle
#define ATT_INVALID_HDL       0x0000

#define ATT_1ST_REQ_START_HDL                          0x0001
#define ATT_1ST_REQ_END_HDL                            0xFFFF


/// macro to fill an attribute characteristic
#define ATT_CHAR(prop, handle, type) {(prop),                                           \
                                     {(uint8_t)((handle)), (uint8_t)((handle) >> 8)},  \
                                     {(uint8_t)((type)), (uint8_t)((type) >> 8)}       };

/// macro to fill an attribute characteristic
#define ATT_CHAR128(prop, handle, type) {(prop),                                         \
                                         {(uint8_t)(handle), (uint8_t)(handle >> 8)}, \
                                         (type) };


/// macro to fill characteristic format
#define ATT_FORMAT(form, exp, unit, name_space, desc) {(form), (exp),                            \
                                                        (uint8_t) (unit), (uint8_t)((unit) >> 8), \
                                                        (name_space),                             \
                                                        (uint8_t) (desc), (uint8_t)((desc) >> 8)  };


/// Offset of value in signed PDU
#define ATT_SIGNED_PDU_VAL_OFFSET       0x03

// 4 last max length bit is kept to determine if attribute has a 128 bits UUID or not.
// and if it's a BT UUID or not
#define ATT_MAX_LENGTH_MASK                    (0x0FFF)
#define ATT_UUID_TYPE_MASK                     (0xF000)
#define ATT_UUID_TYPE_SHIFT                    (12)


/// Attribute Features
#define ATT_SERVER_CONFIG                       0x0001
#define ATT_SERVICE_DISC                        0x0002
#define ATT_RELATIONSHIP_DISC                   0x0004
#define ATT_CHAR_DISC                           0x0008
#define ATT_CHAR_DESC_DISC                      0x0010
#define ATT_RD_CHAR_VALUE                       0x0020
#define ATT_WR_CHAR_VALUE                       0x0040
#define ATT_NOTIF_CHAR_VALUE                    0x0080
#define ATT_IND_CHAR_VALUE                      0x0100
#define ATT_RD_CHAR_DESC                        0x0200
#define ATT_WR_CHAR_DESC                        0x0400

/// Length, number, offset defines
#define ATT_SVC_VALUE_MAX_LEN                   0x0030
#define ATT_CHAR_NAME_MAX_LEN                   0x0030
#define ATT_UUID_16_LEN                         0x0002
#define ATT_UUID_32_LEN                         0x0004
#define ATT_UUID_128_LEN                        0x0010
#define ATT_MAX_NUM_EXT_PROFILE                 0x000A
#define ATT_MAX_NUM_INCL_PROFILE                0x000A
/// offset - l2cap header and ATT code
#define ATT_PDU_DATA_OFFSET                     0x05
/// Characteristic Properties Bit
#define ATT_CHAR_PROP_BCAST                     0x01
#define ATT_CHAR_PROP_RD                        0x02
#define ATT_CHAR_PROP_WR_NO_RESP                0x04
#define ATT_CHAR_PROP_WR                        0x08
#define ATT_CHAR_PROP_NTF                       0x10
#define ATT_CHAR_PROP_IND                       0x20
#define ATT_CHAR_PROP_AUTH                      0x40
#define ATT_CHAR_PROP_EXT_PROP                  0x80
/// Invalid Attribute Handle
#define ATT_INVALID_SEARCH_HANDLE               0x0000
#define ATT_INVALID_HANDLE                      0x0000
/// Read Information Request
#define ATT_UUID_FILTER_0                       0x00
#define ATT_UUID_FILTER_2                       0x02
#define ATT_UUID_FILTER_16                      0x10
/// Read Information Response
#define ATT_FORMAT_16BIT_UUID                   0x01
#define ATT_FORMAT_128BIT_UUID                  0x02
/// For No fix length PDU
#define ATT_HANDLE_LEN                          0x0002
#define ATT_CODE_LEN                            0x0001
#define ATT_CODE_AND_DATA_LEN                   0x0002
#define ATT_CODE_AND_HANDLE_LEN                 0x0003
#define ATT_CODE_AND_HANDLE_LEN_AND_OFFSET      0x0005
#define ATT_SIGNATURE_LEN                       0x0C

/// extended characteristics
#define ATT_EXT_RELIABLE_WRITE                  0x0001
#define ATT_EXT_WRITABLE_AUX                    0x0002
#define ATT_EXT_RFU                             0xFFFC

/// PDU size for error response
#define ATT_ERROR_RESP_LEN                      0x05

/// Offset of value in signed PDU
#define ATT_SIGNED_PDU_VAL_OFFSET               0x03


/// Maximum value length
#define ATT_MAX_VALUE                           (ATT_DEFAULT_MTU+1)


#define ATT_BT_UUID_128 {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
                          0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}



/* Attribute Specification Defines */

/// Attribute Error Code from BT 4.0 specs
enum {
    /// 0x00: No error
    ATT_ERR_NO_ERROR            = 0x00,
    /// 0x01: Handle is invalid
    ATT_ERR_INVALID_HANDLE,
    /// 0x02: Read permission disabled
    ATT_ERR_READ_NOT_PERMITTED,
    /// 0x03: Write permission disabled
    ATT_ERR_WRITE_NOT_PERMITTED,
    /// 0x04: Incorrect PDU
    ATT_ERR_INVALID_PDU,
    /// 0x05: Authentication privilege not enough
    ATT_ERR_INSUFF_AUTHEN,
    /// 0x06: Request not supported or not understood
    ATT_ERR_REQUEST_NOT_SUPPORTED,
    /// 0x07: Incorrect offset value
    ATT_ERR_INVALID_OFFSET,
    /// 0x08: Authorization privilege not enough
    ATT_ERR_INSUFF_AUTHOR,
    /// 0x09: Capacity queue for reliable write reached
    ATT_ERR_PREPARE_QUEUE_FULL,
    /// 0x0A: Attribute requested not existing
    ATT_ERR_ATTRIBUTE_NOT_FOUND,
    /// 0x0B: Attribute requested not long
    ATT_ERR_ATTRIBUTE_NOT_LONG,
    /// 0x0C: Encryption size not sufficient
    ATT_ERR_INSUFF_ENC_KEY_SIZE,
    /// 0x0D: Invalid length of the attribute value
    ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN,
    /// 0x0E: Operation not fit to condition
    ATT_ERR_UNLIKELY_ERR,
    /// 0x0F: Attribute requires encryption before operation
    ATT_ERR_INSUFF_ENC,
    /// 0x10: Attribute grouping not supported
    ATT_ERR_UNSUPP_GRP_TYPE,
    /// 0x11: Resources not sufficient to complete the request
    ATT_ERR_INSUFF_RESOURCE,
    /// 0x80: Application Error
    ATT_ERR_APP_ERROR = 0x80,
    /// Maximum attribute defined errors
    ATT_ERR_LAST
};

/// Common 16-bit Universal Unique Identifier
enum {
    ATT_INVALID_UUID = 0,
    /*----------------- SERVICES ---------------------*/
    /// Generic Access Profile
    ATT_SVC_GENERIC_ACCESS                      = 0x1800,
    /// Attribute Profile
    ATT_SVC_GENERIC_ATTRIBUTE,
    /// Immediate alert Service
    ATT_SVC_IMMEDIATE_ALERT,
    /// Link Loss Service
    ATT_SVC_LINK_LOSS,
    /// Tx Power Service
    ATT_SVC_TX_POWER,
    /// Current Time Service Service
    ATT_SVC_CURRENT_TIME,
    /// Reference Time Update Service
    ATT_SVC_REF_TIME_UPDATE,
    /// Next DST Change Service
    ATT_SVC_NEXT_DST_CHANGE,
    /// Glucose Service
    ATT_SVC_GLUCOSE                             = 0x1808,
    /// Health Thermometer Service
    ATT_SVC_HEALTH_THERMOM                      = 0x1809,
    /// Device Information Service
    ATT_SVC_DEVICE_INFO                         = 0x180A,
    /// Heart Rate Service
    ATT_SVC_HEART_RATE                          = 0x180D,
    /// Phone Alert Status Service
    ATT_SVC_PHONE_ALERT_STATUS,
    /// Battery Service
    ATT_SVC_BATTERY_SERVICE,
    /// Blood Pressure Service
    ATT_SVC_BLOOD_PRESSURE                      = 0x1810,
    /// Alert Notification Service
    ATT_SVC_ALERT_NTF                           = 0x1811,
    /// HID Service
    ATT_SVC_HID                                 = 0x1812,
    /// Scan Parameters Service
    ATT_SVC_SCAN_PARAMETERS                     = 0x1813,
    /// Running Speed and Cadence Service
    ATT_SVC_RUNNING_SPEED_CADENCE               = 0x1814,
    /// Cycling Speed and Cadence Service
    ATT_SVC_CYCLING_SPEED_CADENCE               = 0x1816,
    /// Cycling Power Service
    ATT_SVC_CYCLING_POWER                       = 0x1818,
    /// Location and Navigation Service
    ATT_SVC_LOCATION_AND_NAVIGATION             = 0x1819,
    /// Body Composition Service
    ATT_SVC_BODY_COMPOSITION                    = 0x181B,
    /// User Data Service
    ATT_SVC_USER_DATA                           = 0x181C,
    /// Weight Scale Service
    ATT_SVC_WEIGHT_SCALE                        = 0x181D,
    /// Bond Management Service
    ATT_SVC_BOND_MANAGEMENT                     = 0x181E,

    /*------------------- UNITS ---------------------*/
    /// No defined unit
    ATT_UNIT_UNITLESS                       = 0x2700,
    /// Length Unit - Metre
    ATT_UNIT_METRE,
    //Mass unit - Kilogram
    ATT_UNIT_KG,
    ///Time unit - second
    ATT_UNIT_SECOND,
    ///Electric current unit - Ampere
    ATT_UNIT_AMPERE,
    ///Thermodynamic Temperature unit - Kelvin
    ATT_UNIT_KELVIN,
    /// Amount of substance unit - mole
    ATT_UNIT_MOLE,
    ///Luminous intensity unit - candela
    ATT_UNIT_CANDELA,
    ///Area unit - square metres
    ATT_UNIT_SQ_METRE                       = 0x2710,
    ///Colume unit - cubic metres
    ATT_UNIT_CUBIC_METRE,
    ///Velocity unit - metres per second
    ATT_UNIT_METRE_PER_SECOND,
    ///Acceleration unit - metres per second squared
    ATT_UNIT_METRES_PER_SEC_SQ,
    ///Wavenumber unit - reciprocal metre
    ATT_UNIT_RECIPROCAL_METRE,
    ///Density unit - kilogram per cubic metre
    ATT_UNIT_DENS_KG_PER_CUBIC_METRE,
    ///Surface density unit - kilogram per square metre
    ATT_UNIT_KG_PER_SQ_METRE,
    ///Specific volume unit - cubic metre per kilogram
    ATT_UNIT_CUBIC_METRE_PER_KG,
    ///Current density unit - ampere per square metre
    ATT_UNIT_AMPERE_PER_SQ_METRE,
    ///Magnetic field strength unit - Ampere per metre
    ATT_UNIT_AMPERE_PER_METRE,
    ///Amount concentration unit - mole per cubic metre
    ATT_UNIT_MOLE_PER_CUBIC_METRE,
    ///Mass Concentration unit - kilogram per cubic metre
    ATT_UNIT_MASS_KG_PER_CUBIC_METRE,
    ///Luminance unit - candela per square metre
    ATT_UNIT_CANDELA_PER_SQ_METRE,
    ///Refractive index unit
    ATT_UNIT_REFRACTIVE_INDEX,
    ///Relative permeability unit
    ATT_UNIT_RELATIVE_PERMEABILITY,
    ///Plane angle unit - radian
    ATT_UNIT_RADIAN                         = 0x2720,
    ///Solid angle unit - steradian
    ATT_UNIT_STERADIAN,
    ///Frequency unit - Hertz
    ATT_UNIT_HERTZ,
    ///Force unit - Newton
    ATT_UNIT_NEWTON,
    ///Pressure unit - Pascal
    ATT_UNIT_PASCAL,
    ///Energy unit - Joule
    ATT_UNIT_JOULE,
    ///Power unit - Watt
    ATT_UNIT_WATT,
    ///electric Charge unit - Coulomb
    ATT_UNIT_COULOMB,
    ///Electric potential difference - Volt
    ATT_UNIT_VOLT,
    ///Capacitance unit - Farad
    ATT_UNIT_FARAD,
    ///electric resistance unit - Ohm
    ATT_UNIT_OHM,
    ///Electric conductance - Siemens
    ATT_UNIT_SIEMENS,
    ///Magnetic flux unit - Weber
    ATT_UNIT_WEBER,
    ///Magnetic flux density unit - Tesla
    ATT_UNIT_TESLA,
    ///Inductance unit - Henry
    ATT_UNIT_HENRY,
    ///Temperature unit - degree Celsius
    ATT_UNIT_CELSIUS,
    ///Luminous flux unit - lumen
    ATT_UNIT_LUMEN,
    //Illuminance unit - lux
    ATT_UNIT_LUX,
    ///Activity referred to a radionuclide unit - becquerel
    ATT_UNIT_BECQUEREL,
    ///Absorbed dose unit - Gray
    ATT_UNIT_GRAY,
    ///Dose equivalent unit - Sievert
    ATT_UNIT_SIEVERT,
    ///Catalytic activity unit - Katal
    ATT_UNIT_KATAL,
    ///Synamic viscosity unit - Pascal second
    ATT_UNIT_PASCAL_SECOND                      = 0x2740,
    ///Moment of force unit - Newton metre
    ATT_UNIT_NEWTON_METRE,
    ///surface tension unit - Newton per metre
    ATT_UNIT_NEWTON_PER_METRE,
    ///Angular velocity unit - radian per second
    ATT_UNIT_RADIAN_PER_SECOND,
    ///Angular acceleration unit - radian per second squared
    ATT_UNIT_RADIAN_PER_SECOND_SQ,
    ///Heat flux density unit - Watt per square metre
    ATT_UNIT_WATT_PER_SQ_METRE,
    ///HEat capacity unit - Joule per Kelvin
    ATT_UNIT_JOULE_PER_KELVIN,
    ///Specific heat capacity unit - Joule per kilogram kelvin
    ATT_UNIT_JOULE_PER_KG_KELVIN,
    ///Specific Energy unit - Joule per kilogram
    ATT_UNIT_JOULE_PER_KG,
    ///Thermal conductivity - Watt per metre Kelvin
    ATT_UNIT_WATT_PER_METRE_KELVIN,
    ///Energy Density unit - joule per cubic metre
    ATT_UNIT_JOULE_PER_CUBIC_METRE,
    ///Electric field strength unit - volt per metre
    ATT_UNIT_VOLT_PER_METRE,
    ///Electric charge density unit - coulomb per cubic metre
    ATT_UNIT_COULOMB_PER_CUBIC_METRE,
    ///Surface charge density unit - coulomb per square metre
    ATT_UNIT_SURF_COULOMB_PER_SQ_METRE,
    ///Electric flux density unit - coulomb per square metre
    ATT_UNIT_FLUX_COULOMB_PER_SQ_METRE,
    ///Permittivity unit - farad per metre
    ATT_UNIT_FARAD_PER_METRE,
    ///Permeability unit - henry per metre
    ATT_UNIT_HENRY_PER_METRE,
    ///Molar energy unit - joule per mole
    ATT_UNIT_JOULE_PER_MOLE,
    ///Molar entropy unit - joule per mole kelvin
    ATT_UNIT_JOULE_PER_MOLE_KELVIN,
    ///Exposure unit - coulomb per kilogram
    ATT_UNIT_COULOMB_PER_KG,
    ///Absorbed dose rate unit - gray per second
    ATT_UNIT_GRAY_PER_SECOND,
    ///Radiant intensity unit - watt per steradian
    ATT_UNIT_WATT_PER_STERADIAN,
    ///Radiance unit - watt per square meter steradian
    ATT_UNIT_WATT_PER_SQ_METRE_STERADIAN,
    ///Catalytic activity concentration unit - katal per cubic metre
    ATT_UNIT_KATAL_PER_CUBIC_METRE,
    ///Time unit - minute
    ATT_UNIT_MINUTE                         = 0x2760,
    ///Time unit - hour
    ATT_UNIT_HOUR,
    ///Time unit - day
    ATT_UNIT_DAY,
    ///Plane angle unit - degree
    ATT_UNIT_ANGLE_DEGREE,
    ///Plane angle unit - minute
    ATT_UNIT_ANGLE_MINUTE,
    ///Plane angle unit - second
    ATT_UNIT_ANGLE_SECOND,
    ///Area unit - hectare
    ATT_UNIT_HECTARE,
    ///Volume unit - litre
    ATT_UNIT_LITRE,
    ///Mass unit - tonne
    ATT_UNIT_TONNE,
    ///Pressure unit - bar
    ATT_UNIT_BAR                            = 0x2780,
    ///Pressure unit - millimetre of mercury
    ATT_UNIT_MM_MERCURY,
    ///Length unit - angstrom
    ATT_UNIT_ANGSTROM,
    ///Length unit - nautical mile
    ATT_UNIT_NAUTICAL_MILE,
    ///Area unit - barn
    ATT_UNIT_BARN,
    ///Velocity unit - knot
    ATT_UNIT_KNOT,
    ///Logarithmic radio quantity unit - neper
    ATT_UNIT_NEPER,
    ///Logarithmic radio quantity unit - bel
    ATT_UNIT_BEL,
    ///Length unit - yard
    ATT_UNIT_YARD                           = 0x27A0,
    ///Length unit - parsec
    ATT_UNIT_PARSEC,
    ///length unit - inch
    ATT_UNIT_INCH,
    ///length unit - foot
    ATT_UNIT_FOOT,
    ///length unit - mile
    ATT_UNIT_MILE,
    ///pressure unit - pound-force per square inch
    ATT_UNIT_POUND_FORCE_PER_SQ_INCH,
    ///velocity unit - kilometre per hour
    ATT_UNIT_KM_PER_HOUR,
    ///velocity unit - mile per hour
    ATT_UNIT_MILE_PER_HOUR,
    ///angular velocity unit - revolution per minute
    ATT_UNIT_REVOLUTION_PER_MINUTE,
    ///energy unit - gram calorie
    ATT_UNIT_GRAM_CALORIE,
    ///energy unit - kilogram calorie
    ATT_UNIT_KG_CALORIE,
    /// energy unit - kilowatt hour
    ATT_UNIT_KILOWATT_HOUR,
    ///thermodynamic temperature unit - degree Fahrenheit
    ATT_UNIT_FAHRENHEIT,
    ///percentage
    ATT_UNIT_PERCENTAGE,
    ///per mille
    ATT_UNIT_PER_MILLE,
    ///period unit - beats per minute)
    ATT_UNIT_BEATS_PER_MINUTE,
    ///electric charge unit - ampere hours
    ATT_UNIT_AMPERE_HOURS,
    ///mass density unit - milligram per decilitre
    ATT_UNIT_MILLIGRAM_PER_DECILITRE,
    ///mass density unit - millimole per litre
    ATT_UNIT_MILLIMOLE_PER_LITRE,
    ///time unit - year
    ATT_UNIT_YEAR,
    ////time unit - month
    ATT_UNIT_MONTH,


    /*---------------- DECLARATIONS -----------------*/
    /// Primary service Declaration
    ATT_DECL_PRIMARY_SERVICE                     = 0x2800,
    /// Secondary service Declaration
    ATT_DECL_SECONDARY_SERVICE,
    /// Include Declaration
    ATT_DECL_INCLUDE,
    /// Characteristic Declaration
    ATT_DECL_CHARACTERISTIC,


    /*----------------- DESCRIPTORS -----------------*/
    /// Characteristic extended properties
    ATT_DESC_CHAR_EXT_PROPERTIES                 = 0x2900,
    /// Characteristic user description
    ATT_DESC_CHAR_USER_DESCRIPTION,
    /// Client characteristic configuration
    ATT_DESC_CLIENT_CHAR_CFG,
    /// Server characteristic configuration
    ATT_DESC_SERVER_CHAR_CFG,
    /// Characteristic Presentation Format
    ATT_DESC_CHAR_PRES_FORMAT,
    /// Characteristic Aggregate Format
    ATT_DESC_CHAR_AGGREGATE_FORMAT,
    /// Valid Range
    ATT_DESC_VALID_RANGE,
    /// External Report Reference
    ATT_DESC_EXT_REPORT_REF,
    /// Report Reference
    ATT_DESC_REPORT_REF,


    /*--------------- CHARACTERISTICS ---------------*/
    /// Device name
    ATT_CHAR_DEVICE_NAME                        = 0x2A00,
    /// Appearance
    ATT_CHAR_APPEARANCE                         = 0x2A01,
    /// Privacy flag
    ATT_CHAR_PRIVACY_FLAG                       = 0x2A02,
    /// Reconnection address
    ATT_CHAR_RECONNECTION_ADDR                  = 0x2A03,
    /// Peripheral preferred connection parameters
    ATT_CHAR_PERIPH_PREF_CON_PARAM              = 0x2A04,
    /// Service handles changed
    ATT_CHAR_SERVICE_CHANGED                    = 0x2A05,
    /// Alert Level characteristic
    ATT_CHAR_ALERT_LEVEL                        = 0x2A06,
    /// Tx Power Level
    ATT_CHAR_TX_POWER_LEVEL                     = 0x2A07,
    /// Date Time
    ATT_CHAR_DATE_TIME                          = 0x2A08,
    /// Day of Week
    ATT_CHAR_DAY_WEEK                           = 0x2A09,
    /// Day Date Time
    ATT_CHAR_DAY_DATE_TIME                      = 0x2A0A,
    /// Exact time 256
    ATT_CHAR_EXACT_TIME_256                     = 0x2A0C,
    /// DST Offset
    ATT_CHAR_DST_OFFSET                         = 0x2A0D,
    /// Time zone
    ATT_CHAR_TIME_ZONE                          = 0x2A0E,
    /// Local time Information
    ATT_CHAR_LOCAL_TIME_INFO                    = 0x2A0F,
    /// Time with DST
    ATT_CHAR_TIME_WITH_DST                      = 0x2A11,
    /// Time Accuracy
    ATT_CHAR_TIME_ACCURACY                      = 0x2A12,
    ///Time Source
    ATT_CHAR_TIME_SOURCE                        = 0x2A13,
    /// Reference Time Information
    ATT_CHAR_REFERENCE_TIME_INFO                = 0x2A14,
    /// Time Update Control Point
    ATT_CHAR_TIME_UPDATE_CNTL_POINT             = 0x2A16,
    /// Time Update State
    ATT_CHAR_TIME_UPDATE_STATE                  = 0x2A17,
    /// Glucose Measurement
    ATT_CHAR_GLUCOSE_MEAS                       = 0x2A18,
    /// Battery Level
    ATT_CHAR_BATTERY_LEVEL                      = 0x2A19,
    /// Temperature Measurement
    ATT_CHAR_TEMPERATURE_MEAS                   = 0x2A1C,
    /// Temperature Type
    ATT_CHAR_TEMPERATURE_TYPE                   = 0x2A1D,
    /// Intermediate Temperature
    ATT_CHAR_INTERMED_TEMPERATURE               = 0x2A1E,
    /// Measurement Interval
    ATT_CHAR_MEAS_INTERVAL                      = 0x2A21,
    /// Boot Keyboard Input Report
    ATT_CHAR_BOOT_KB_IN_REPORT                  = 0x2A22,
    /// System ID
    ATT_CHAR_SYS_ID                             = 0x2A23,
    /// Model Number String
    ATT_CHAR_MODEL_NB                           = 0x2A24,
    /// Serial Number String
    ATT_CHAR_SERIAL_NB                          = 0x2A25,
    /// Firmware Revision String
    ATT_CHAR_FW_REV                             = 0x2A26,
    /// Hardware revision String
    ATT_CHAR_HW_REV                             = 0x2A27,
    /// Software Revision String
    ATT_CHAR_SW_REV                             = 0x2A28,
    /// Manufacturer Name String
    ATT_CHAR_MANUF_NAME                         = 0x2A29,
    /// IEEE Regulatory Certification Data List
    ATT_CHAR_IEEE_CERTIF                        = 0x2A2A,
    /// CT Time
    ATT_CHAR_CT_TIME                            = 0x2A2B,
    /// Scan Refresh
    ATT_CHAR_SCAN_REFRESH                       = 0x2A31,
    /// Boot Keyboard Output Report
    ATT_CHAR_BOOT_KB_OUT_REPORT                 = 0x2A32,
    /// Boot Mouse Input Report
    ATT_CHAR_BOOT_MOUSE_IN_REPORT               = 0x2A33,
    /// Glucose Measurement Context
    ATT_CHAR_GLUCOSE_MEAS_CTX                   = 0x2A34,
    /// Blood Pressure Measurement
    ATT_CHAR_BLOOD_PRESSURE_MEAS                = 0x2A35,
    /// Intermediate Cuff Pressure
    ATT_CHAR_INTERMEDIATE_CUFF_PRESSURE         = 0x2A36,
    /// Heart Rate Measurement
    ATT_CHAR_HEART_RATE_MEAS                    = 0x2A37,
    /// Body Sensor Location
    ATT_CHAR_BODY_SENSOR_LOCATION               = 0x2A38,
    /// Heart Rate Control Point
    ATT_CHAR_HEART_RATE_CNTL_POINT              = 0x2A39,
    /// Alert Status
    ATT_CHAR_ALERT_STATUS                       = 0x2A3F,
    /// Ringer Control Point
    ATT_CHAR_RINGER_CNTL_POINT                  = 0x2A40,
    /// Ringer Setting
    ATT_CHAR_RINGER_SETTING                     = 0x2A41,
    /// Alert Category ID Bit Mask
    ATT_CHAR_ALERT_CAT_ID_BIT_MASK              = 0x2A42,
    /// Alert Category ID
    ATT_CHAR_ALERT_CAT_ID                       = 0x2A43,
    /// Alert Notification Control Point
    ATT_CHAR_ALERT_NTF_CTNL_PT                  = 0x2A44,
    /// Unread Alert Status
    ATT_CHAR_UNREAD_ALERT_STATUS                = 0x2A45,
    /// New Alert
    ATT_CHAR_NEW_ALERT                          = 0x2A46,
    /// Supported New Alert Category
    ATT_CHAR_SUP_NEW_ALERT_CAT                  = 0x2A47,
    /// Supported Unread Alert Category
    ATT_CHAR_SUP_UNREAD_ALERT_CAT               = 0x2A48,
    /// Blood Pressure Feature
    ATT_CHAR_BLOOD_PRESSURE_FEATURE             = 0x2A49,
    /// HID Information
    ATT_CHAR_HID_INFO                           = 0x2A4A,
    /// Report Map
    ATT_CHAR_REPORT_MAP                         = 0x2A4B,
    /// HID Control Point
    ATT_CHAR_HID_CTNL_PT                        = 0x2A4C,
    /// Report
    ATT_CHAR_REPORT                             = 0x2A4D,
    /// Protocol Mode
    ATT_CHAR_PROTOCOL_MODE                      = 0x2A4E,
    /// Scan Interval Window
    ATT_CHAR_SCAN_INTV_WD                       = 0x2A4F,
    /// PnP ID
    ATT_CHAR_PNP_ID                             = 0x2A50,
    /// Glucose Feature
    ATT_CHAR_GLUCOSE_FEATURE                    = 0x2A51,
    /// Record access control point
    ATT_CHAR_REC_ACCESS_CTRL_PT                 = 0x2A52,
    /// RSC Measurement
    ATT_CHAR_RSC_MEAS                           = 0x2A53,
    /// RSC Feature
    ATT_CHAR_RSC_FEAT                           = 0x2A54,
    /// SC Control Point
    ATT_CHAR_SC_CNTL_PT                         = 0x2A55,
    /// CSC Measurement
    ATT_CHAR_CSC_MEAS                           = 0x2A5B,
    /// CSC Feature
    ATT_CHAR_CSC_FEAT                           = 0x2A5C,
    /// Sensor Location
    ATT_CHAR_SENSOR_LOC                         = 0x2A5D,
    /// CP Measurement
    ATT_CHAR_CP_MEAS                            = 0x2A63,
    /// CP Vector
    ATT_CHAR_CP_VECTOR                          = 0x2A64,
    /// CP Feature
    ATT_CHAR_CP_FEAT                            = 0x2A65,
    /// CP Control Point
    ATT_CHAR_CP_CNTL_PT                         = 0x2A66,
    /// Location and Speed
    ATT_CHAR_LOC_SPEED                          = 0x2A67,
    /// Navigation
    ATT_CHAR_NAVIGATION                         = 0x2A68,
    /// Position Quality
    ATT_CHAR_POS_QUALITY                        = 0x2A69,
    /// LN Feature
    ATT_CHAR_LN_FEAT                            = 0x2A6A,
    /// LN Control Point
    ATT_CHAR_LN_CNTL_PT                         = 0x2A6B,

    /// Weight Scale Feature
    ATT_CHAR_WS_FEAT                            = 0x2A9E,
    /// Weight Scale Measurement
    ATT_CHAR_WS_MEAS                            = 0x2A9D,
    /// User Aerobic Heart Rate Lower Limit
    ATT_CHAR_UDS_USER_AEROBIC_HR_LOW_LIM        = 0x2A7E,
    /// User Aerobic Threshold
    ATT_CHAR_UDS_USER_AEROBIC_THRESHOLD         = 0x2A7F,
    /// User age
    ATT_CHAR_UDS_USER_AGE                       = 0x2A80,
    /// User Anaerobic Heart Rate Lower Limit
    ATT_CHAR_UDS_USER_ANAEROBIC_HR_LOW_LIM      = 0x2A81,
    /// User Anaerobic Heart Rate Upper Limit
    ATT_CHAR_UDS_USER_ANAEROBIC_HR_UP_LIM       = 0x2A82,
    /// User Anaerobic Threshold
    ATT_CHAR_UDS_USER_ANAEROBIC_THRESHOLD       = 0x2A83,
    /// User Aerobic Heart Rate Upper Limit
    ATT_CHAR_UDS_USER_AEROBIC_HR_UP_LIM         = 0x2A84,
    /// User Date of Birth
    ATT_CHAR_UDS_USER_DATE_OF_BIRTH             = 0x2A85,
    /// User Date of Threshold Assessment
    ATT_CHAR_UDS_USER_DATE_OF_THRESHOLD_ASS     = 0x2A86,
    /// User Email Address
    ATT_CHAR_UDS_USER_EMAIL_ADDR                = 0x2A87,
    /// User Fat Burn Heart Rate Lower Limit
    ATT_CHAR_UDS_USER_FAT_BURN_HR_LOW_LIM       = 0x2A88,
    /// User Fat Burn Heart Rate Upper Limit
    ATT_CHAR_UDS_USER_FAT_BURN_HR_UP_LIM        = 0x2A89,
    /// User First Name
    ATT_CHAR_UDS_USER_FIRST_NAME                = 0x2A8A,
    /// User Five Zone Heart Rate Limits
    ATT_CHAR_UDS_USER_5ZONE_HR_LIM              = 0x2A8B,
    /// User Gender
    ATT_CHAR_UDS_USER_GENDER                    = 0x2A8C,
    /// User Heart Rate Max
    ATT_CHAR_UDS_USER_HEART_RATE_MAX            = 0x2A8D,
    /// User height
    ATT_CHAR_UDS_USER_HEIGHT                    = 0x2A8E,
    /// User Hip Circumference
    ATT_CHAR_UDS_USER_HIP_CIRCUMFERENCE         = 0x2A8F,
    /// User Last Name
    ATT_CHAR_UDS_USER_LAST_NAME                 = 0x2A90,
    /// User Maximum Recommended Heart Rate
    ATT_CHAR_UDS_USER_MAX_REC_HEART_RATE        = 0x2A91,
    /// User Resting Heart Rate
    ATT_CHAR_UDS_USER_RESTING_HEART_RATE        = 0x2A92,
    /// User Sport Type for Aerobic and Anaerobic Thresholds
    ATT_CHAR_UDS_USER_THRESHOLDS_SPORT_TYPE     = 0x2A93,
    /// User Three Zone Heart Rate Limits
    ATT_CHAR_UDS_USER_3ZONE_HR_LIM              = 0x2A94,
    /// User Two Zone Heart Rate Limit
    ATT_CHAR_UDS_USER_2ZONE_HR_LIM              = 0x2A95,
    /// User VO2 Max
    ATT_CHAR_UDS_USER_VO2_MAX                   = 0x2A96,
    /// User Waist Circumference
    ATT_CHAR_UDS_USER_WAIST_CIRCUMFERENCE       = 0x2A97,
    /// User Weight
    ATT_CHAR_UDS_USER_WEIGHT                    = 0x2A98,
    /// User database change increment
    ATT_CHAR_UDS_USER_DB_CHANGE_INCR            = 0x2A99,
    /// User Data Index
    ATT_CHAR_UDS_USER_INDEX                     = 0x2A9A,
    /// Body Composition Feature
    ATT_CHAR_BCS_FEATURE                        = 0x2A9B,
    /// Body Composition Measurement
    ATT_CHAR_BCS_MEAS                           = 0x2A9C,
    /// User Data Control Point
    ATT_CHAR_UDS_USER_CTRL_PT                   = 0x2A9F,
    /// User Language
    ATT_CHAR_UDS_USER_LANGUAGE                  = 0x2AA2,
    // Bond Management Control Point
    ATT_CHAR_BM_CTRL_PT                         = 0x2AA4,
    // Bond Management Feature
    ATT_CHAR_BM_FEAT                            = 0x2AA5,
    /// Last define
    ATT_LAST
};

/// Format for Characteristic Presentation
enum {
    /// unsigned 1-bit: true or false
    ATT_FORMAT_BOOL     = 0x01,
    /// unsigned 2-bit integer
    ATT_FORMAT_2BIT,
    /// unsigned 4-bit integer
    ATT_FORMAT_NIBBLE,
    /// unsigned 8-bit integer
    ATT_FORMAT_UINT8,
    /// unsigned 12-bit integer
    ATT_FORMAT_UINT12,
    /// unsigned 16-bit integer
    ATT_FORMAT_UINT16,
    /// unsigned 24-bit integer
    ATT_FORMAT_UINT24,
    /// unsigned 32-bit integer
    ATT_FORMAT_UINT32,
    /// unsigned 48-bit integer
    ATT_FORMAT_UINT48,
    /// unsigned 64-bit integer
    ATT_FORMAT_UINT64,
    /// unsigned 128-bit integer
    ATT_FORMAT_UINT128,
    /// signed 8-bit integer
    ATT_FORMAT_SINT8,
    /// signed 12-bit integer
    ATT_FORMAT_SINT12,
    /// signed 16-bit integer
    ATT_FORMAT_SINT16,
    /// signed 24-bit integer
    ATT_FORMAT_SINT24,
    /// signed 32-bit integer
    ATT_FORMAT_SINT32,
    /// signed 48-bit integer
    ATT_FORMAT_SINT48,
    /// signed 64-bit integer
    ATT_FORMAT_SINT64,
    /// signed 128-bit integer
    ATT_FORMAT_SINT128,
    /// IEEE-754 32-bit floating point
    ATT_FORMAT_FLOAT32,
    /// IEEE-754 64-bit floating point
    ATT_FORMAT_FLOAT64,
    /// IEEE-11073 16-bit SFLOAT
    ATT_FORMAT_SFLOAT,
    /// IEEE-11073 32-bit FLOAT
    ATT_FORMAT_FLOAT,
    /// IEEE-20601 format
    ATT_FORMAT_DUINT16,
    /// UTF-8 string
    ATT_FORMAT_UTF8S,
    /// UTF-16 string
    ATT_FORMAT_UTF16S,
    /// Opaque structure
    ATT_FORMAT_STRUCT,
    /// Last format
    ATT_FORMAT_LAST
};

/*
 * Type Definition
 ****************************************************************************************
 */

/// Attribute length type
typedef uint16_t att_size_t;


/// UUID - 128-bit type
struct att_uuid_128
{
    /// 128-bit UUID
    uint8_t uuid[ATT_UUID_128_LEN];
};

/// UUID - 32-bit type
struct att_uuid_32
{
    /// 32-bit UUID
    uint8_t uuid[ATT_UUID_32_LEN];
};



/// Characteristic Value Descriptor
struct att_char_desc
{
    /// properties
    uint8_t prop;
    /// attribute handle
    uint8_t attr_hdl[ATT_HANDLE_LEN];
    /// attribute type
    uint8_t attr_type[ATT_UUID_16_LEN];
};

/// Characteristic Value Descriptor
struct att_char128_desc
{
    /// properties
    uint8_t prop;
    /// attribute handle
    uint8_t attr_hdl[ATT_HANDLE_LEN];
    /// attribute type
    uint8_t attr_type[ATT_UUID_128_LEN];
};

/// Service Value Descriptor - 16-bit
typedef uint16_t att_svc_desc_t;

/// include service entry element
struct att_incl_desc
{
    /// start handle value of included service
    uint16_t start_hdl;
    /// end handle value of included service
    uint16_t end_hdl;
    /// attribute value UUID
    uint16_t uuid;
};

/// include service entry element
struct att_incl128_desc
{
    /// start handle value of included service
    uint16_t start_hdl;
    /// end handle value of included service
    uint16_t end_hdl;
};

/// Slave preferred connection parameters
struct att_slv_pref
{
    /// Connection interval minimum
    uint16_t con_intv_min;
    /// Connection interval maximum
    uint16_t con_intv_max;
    /// Slave latency
    uint16_t slave_latency;
    /// Connection supervision timeout multiplier
    uint16_t conn_timeout;
};


/// Service Changed type definition
struct att_svc_changed
{
    /// Service start handle which changed
    uint16_t start_hdl;
    /// Service end handle which changed
    uint16_t end_hdl;
};
/// @} ATT
#endif // ATT_H_
