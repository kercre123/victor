/****************************************************************************\
* Copyright (C) 2016 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#pragma once

#include <config/ModuleConfig.hpp>

namespace royale
{
    namespace config
    {
        /**
        * The module configurations are not exported directly from the library.
        * The full list is available via royale::config::getUsbProbeDataRoyale()
        */
        namespace moduleconfig
        {
            /**
            * Configuration for the PicoS with Enclustra firmware.
            */
            extern const royale::config::ModuleConfig PicoS;

            /**
            * Configuration for the PicoFlexx with Enclustra firmware.
            */
            extern const royale::config::ModuleConfig PicoFlexxU6;

            /**
            * Configuration for the PicoFlexx with Enclustra firmware, and additional calibration at
            * 15 megahertz.
            */
            extern const royale::config::ModuleConfig PicoFlexx15MHz;

            /**
            * Configuration for the PicoFlexx with Enclustra firmware, and 940nm illumination.
            */
            extern const royale::config::ModuleConfig PicoFlexx940nm;

            /**
            * Configuration for the EvalBoard with Enclustra firmware.
            */
            extern const royale::config::ModuleConfig EvalBoard;

            /**
            * This is the fallback configuration for the Animator CX3 bring-up board with both UVC
            * and Amundsen firmware.
            *
            * Users of the bring-up board are recommended to copy this config to a new file, assign
            * a product identifier to the device, and configure the ModuleConfigFactoryAnimatorBoard
            * to detect that new hardware by the product identifier and use the new file.  This is
            * not necessary, but is a good practice for handling multiple devices instead of
            * altering the AnimatorDefault for each one.
            *
            * This default is able to use the imager with any access level, unlike the other
            * xxxDefault configs which typically use an eye-safe-unless-level-three fallback.
            */
            extern const royale::config::ModuleConfig AnimatorDefault;

            /**
            * Configuration for the Skylla camera prototype module connected via UVC.
            * (The device uses a CX3-based USB adaptor).
            * Variant 1 :
            * K6 / 60x45 / 850nm / 96 / LE
            */
            extern const royale::config::ModuleConfig Skylla1;

            /**
            * Configuration for the Skylla camera prototype module connected via UVC.
            * (The device uses a CX3-based USB adaptor).
            * Variant 2 :
            * K6 / 60x45 / 850nm / 101 / PO
            */
            extern const royale::config::ModuleConfig Skylla2;

            /**
            * Configuration for the Skylla camera prototype module connected via UVC.
            * (The device uses a CX3-based USB adaptor).
            * Variant 3 :
            * K6 / 60x45 / 945nm / 101 / PO
            */
            extern const royale::config::ModuleConfig Skylla3;

            /**
            * Configuration for the Skylla camera prototype module connected via UVC.
            * (The device uses a CX3-based USB adaptor).
            * Default variant with only a calibration use case.
            */
            extern const royale::config::ModuleConfig SkyllaDefault;

            /**
            * Configuration for the Charybdis camera module connected via UVC.
            * (The device uses a CX3-based USB adaptor).
            */
            extern const royale::config::ModuleConfig Charybdis;

            /**
            * Configuration for the Charybdis camera module connected via UVC.
            * (The device uses a CX3-based USB adaptor).
            * Default variant with only one calibration use case.
            */
            extern const royale::config::ModuleConfig CharybdisDefault;

            /**
            * Configuration for pico maxx (first diffuser)
            */
            extern const royale::config::ModuleConfig PicoMaxx1;

            /**
            * Configuration for pico maxx (second diffuser)
            */
            extern const royale::config::ModuleConfig PicoMaxx2;

            /**
            * Configuration for pico maxx 850nm rev 4 with glass lens
            */
            extern const royale::config::ModuleConfig PicoMaxx850nmGlass;

            /**
            * Configuration for pico maxx 940nm
            */
            extern const royale::config::ModuleConfig PicoMaxx940nm;

            /**
            * Default configuration for pico maxx only one calibration
            * use case
            */
            extern const royale::config::ModuleConfig PicoMaxxDefault;

            /**
            * Configuration for pico monstar (first diffuser)
            */
            extern const royale::config::ModuleConfig PicoMonstar1;

            /**
            * Configuration for pico monstar (second diffuser)
            */
            extern const royale::config::ModuleConfig PicoMonstar2;

            /**
            * Configuration for pico monstar 850nm rev 4 with glass lens
            */
            extern const royale::config::ModuleConfig PicoMonstar850nmGlass;

            /**
            * Configuration for pico monstar 940nm
            */
            extern const royale::config::ModuleConfig PicoMonstar940nm;

            /**
            * Default configuration for pico monstar with only one calibration
            * use case
            */
            extern const royale::config::ModuleConfig PicoMonstarDefault;

            /**
            * Configuration for the Daedalus camera module connected via UVC.
            * Variant 1 :
            * K6 / 60x45 / 850nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Daedalus1;

            /**
            * Configuration for the Daedalus camera module connected via UVC.
            * Variant 2 :
            * K7L / 60x45 / 850nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Daedalus2;

            /**
            * Configuration for the Daedalus camera module connected via UVC.
            * Variant 3 :
            * K8 / 90x70 / 850nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Daedalus3;

            /**
            * Configuration for the Daedalus camera module connected via UVC.
            * Variant 4 :
            * K6 / 60x45 / 945nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Daedalus4;

            /**
            * Configuration for the Daedalus camera module connected via UVC.
            * Variant 5 :
            * This variant is laser class 4!
            * K6 / 60x45 / 945nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Daedalus5;

            /**
            * Default configuration for a Daedalus camera module where no product
            * identifier was found or doesn't match the other variants. This only
            * offers a calibration use case.
            */
            extern const royale::config::ModuleConfig DaedalusDefault;

            /**
            * Configuration for the Alea camera modules with 850nm VCSEL.
            * Variant 1 + 2:
            * 60x45 / 850nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Alea850nm;

            /**
            * Configuration for the Alea camera modules with 945nm VCSEL.
            * Variant 3 + 4:
            * 60x45 / 945nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Alea945nm;

            /**
            * Default configuration for an Alea camera module where no product
            * identifier was found or doesn't match the other variants. This only
            * offers a calibration use case.
            */
            extern const royale::config::ModuleConfig AleaDefault;

            /**
            * Configuration for the Apollo camera module connected via UVC.
            * (The device uses a CX3-based USB adapter).
            */
            extern const royale::config::ModuleConfig Apollo;

            /**
            * Configuration for the Salome camera modules with 940nm VCSel
            * K9 / 60x45 / 945nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Salome940nm;

            /**
            * Configuration for the Salome Rev 2 camera modules with 940nm VCSel
            * K9 or Leica / 60x45 / 945nm / 202 / PO / ICM or ECM
            */
            extern const royale::config::ModuleConfig Salome2Rev940nm;

            /**
            * Default configuration for the Salome camera modules that only
            * offers a calibration use case
            */
            extern const royale::config::ModuleConfig SalomeDefault;

            /**
            * Configuration for the Lewis camera modules
            */
            extern const royale::config::ModuleConfig FacePlusLewis;

            /**
            * Default configuration for the FacePlus camera modules
            */
            extern const royale::config::ModuleConfig FacePlusDefault;

            /**
            * Default configuration for the Williams camera modules
            * (FacePlus 1.5.0 940nm)
            */
            extern const royale::config::ModuleConfig FacePlusWilliams;

            /**
            * Default configuration for the Cuthbert camera modules
            * (FacePlus 1.6.1 850nm)
            */
            extern const royale::config::ModuleConfig FacePlusCuthbert;

            /**
            * Configuration for the PP3 camera modules
            */
            extern const royale::config::ModuleConfig FacePlusPP3;

            /**
            * Configuration for the Equinox camera modules with 940nm VCSel
            * Newmax OTS / 945nm / 60x45 / 404E / PO
            */
            extern const royale::config::ModuleConfig Equinox940nm;

            /**
            * Default configuration for the Equinox camera modules that only
            * offers a calibration use case
            */
            extern const royale::config::ModuleConfig EquinoxDefault;

            /**
            * Configuration for X1 850nm 2W
            */
            extern const royale::config::ModuleConfig X18502W;

            /**
            * Configuration for the F1 module
            */
            extern const royale::config::ModuleConfig F18501W;

            /**
            * Configuration for the Selene camera modules with ICM
            */
            extern const royale::config::ModuleConfig SeleneIcm;

            /**
            * Default configuration for the Selene camera modules that only
            * offers a calibration use case
            */
            extern const royale::config::ModuleConfig SeleneDefault;

            /**
            * Configuration for the Orpheus camera modules
            */
            extern const royale::config::ModuleConfig Orpheus;

            /**
            * Default configuration for the Orpheus camera modules that only
            * offers a calibration use case
            */
            extern const royale::config::ModuleConfig OrpheusDefault;

            /**
            * Default configuration for the Orat45 camera module
            */
            extern const royale::config::ModuleConfig Orat45;

            /**
            * Default configuration for an unknown pmd module
            */
            extern const royale::config::ModuleConfig PmdModuleDefault;

            /**
            * Default configuration for the MTT016 camera module
            */
            extern const royale::config::ModuleConfig MTT016;
        }
    }
}
