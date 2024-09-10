#ifndef WAVESTALBES_H_INCLUDED
#define WAVESTALBES_H_INCLUDED

#include <vector>


#define TRIANGLE_WAVETABLE
#define SAWTOOTH_WAVETABLE
#define REVERSE_SAWTOOTH_WAVETABLE
#define SQUARE_WAVETABLE
#define TANGENT_WAVETABLE
#define SINC_WAVETABLE
#define BANDLIMITED_IMPULSE_WAVETABLE
#define BANDLIMITED_SAWTOOTH_WAVETABLE
#define BANDLIMITED_SQUARE_WAVETABLE

namespace Wavetables {

	//this wavetable must remain as the system defaults to this one.
	extern const std::vector<float> sinTable;
	extern const std::vector<float> sinTableInterp0;
	extern const std::vector<float> sinTableInterp1;
	extern const std::vector<float> sinTableInterp2;
	extern const std::vector<float> sinTableInterp3;

#ifdef TRIANGLE_WAVETABLE
	extern const std::vector<float> triangleTable;
	extern const std::vector<float> triangleTableInterp0;
	extern const std::vector<float> triangleTableInterp1;
	extern const std::vector<float> triangleTableInterp2;
	extern const std::vector<float> triangleTableInterp3;
#endif

#ifdef SAWTOOTH_WAVETABLE
	extern const std::vector<float> sawtoothTable;
	extern const std::vector<float> sawtoothTableInterp0;
	extern const std::vector<float> sawtoothTableInterp1;
	extern const std::vector<float> sawtoothTableInterp2;
	extern const std::vector<float> sawtoothTableInterp3;
#endif

#ifdef REVERSE_SAWTOOTH_WAVETABLE
	extern const std::vector<float> reverseSawtoothTable;
	extern const std::vector<float> reverseSawtoothTableInterp0;
	extern const std::vector<float> reverseSawtoothTableInterp1;
	extern const std::vector<float> reverseSawtoothTableInterp2;
	extern const std::vector<float> reverseSawtoothTableInterp3;
#endif

#ifdef SQUARE_WAVETABLE
	extern const std::vector<float> squareTable;
	extern const std::vector<float> squareTableInterp0;
	extern const std::vector<float> squareTableInterp1;
	extern const std::vector<float> squareTableInterp2;
	extern const std::vector<float> squareTableInterp3;
#endif

#ifdef TANGENT_WAVETABLE
	extern const std::vector<float> tangentTable;
	extern const std::vector<float> tangentTableInterp0;
	extern const std::vector<float> tangentTableInterp1;
	extern const std::vector<float> tangentTableInterp2;
	extern const std::vector<float> tangentTableInterp3;
#endif

#ifdef SINC_WAVETABLE
	extern const std::vector<float> sincTable;
	extern const std::vector<float> sincTableInterp0;
	extern const std::vector<float> sincTableInterp1;
	extern const std::vector<float> sincTableInterp2;
	extern const std::vector<float> sincTableInterp3;
#endif

#ifdef BANDLIMITED_IMPULSE_WAVETABLE
	extern const std::vector<float> bandlimitedImpulseTable;
	extern const std::vector<float> bandlimitedImpulseTableInterp0;
	extern const std::vector<float> bandlimitedImpulseTableInterp1;
	extern const std::vector<float> bandlimitedImpulseTableInterp2;
	extern const std::vector<float> bandlimitedImpulseTableInterp3;
#endif

#ifdef BANDLIMITED_SAWTOOTH_WAVETABLE
	extern const std::vector<float> bandlimitedSawtoothTable;
	extern const std::vector<float> bandlimitedSawtoothTableInterp0;
	extern const std::vector<float> bandlimitedSawtoothTableInterp1;
	extern const std::vector<float> bandlimitedSawtoothTableInterp2;
	extern const std::vector<float> bandlimitedSawtoothTableInterp3;
#endif

#ifdef BANDLIMITED_SQUARE_WAVETABLE
	extern const std::vector<float> bandlimitedSquareTable;
	extern const std::vector<float> bandlimitedSquareTableInterp0;
	extern const std::vector<float> bandlimitedSquareTableInterp1;
	extern const std::vector<float> bandlimitedSquareTableInterp2;
	extern const std::vector<float> bandlimitedSquareTableInterp3;
#endif
}


#endif 
