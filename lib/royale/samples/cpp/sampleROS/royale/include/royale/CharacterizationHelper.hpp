/****************************************************************************\
 * Copyright (C) 2017 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <usecase/UseCase.hpp>

namespace royale
{
    namespace config
    {
        class CharacterizationHelper
        {
        public:
            // Create a characterization use case with the specified frequency
            // All the characterizations should happen at 5fps with 4 phases
            // and only raw data is needed (will be discarded anyway)
            // Level 3 only!!!
            static royale::usecase::UseCase createCharacterizationUseCase (double frequency);
        };
    }
}
