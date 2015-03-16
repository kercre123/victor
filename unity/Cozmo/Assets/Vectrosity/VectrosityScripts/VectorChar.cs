using UnityEngine;

namespace Vectrosity {

public class VectorChar {
	public const int numberOfCharacters = 256;
	static Vector2[][] points;
			
	public static Vector2[][] data {
		get {
			if (points == null) {
				points = new Vector2[numberOfCharacters][];
				// !
				points[33] = new Vector2[] {new Vector2(0, -.9f), new Vector2(0, -1), new Vector2(0, 0), new Vector2(0, -.75f)};
				// "
				points[34] = new Vector2[] {new Vector2(.15f, 0), new Vector2(.15f, -.25f), new Vector2(.45f, -.25f), new Vector2(.45f, 0)};
				// #
				points[35] = new Vector2[] {new Vector2(.2f, 0), new Vector2(.2f, -1f), new Vector2(0, -.33f), new Vector2(.6f, -.33f), new Vector2(.4f, 0), new Vector2(.4f, -1f), new Vector2(0, -.66f), new Vector2(.6f, -.66f)};
				// %
				points[37] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -.25f), new Vector2(.15f, 0), new Vector2(.15f, -.25f), new Vector2(0, -.25f), new Vector2(.15f, -.25f), new Vector2(0, 0), new Vector2(.15f, 0), new Vector2(.6f, -.75f), new Vector2(.45f, -.75f), new Vector2(.6f, -1), new Vector2(.45f, -1), new Vector2(.45f, -1), new Vector2(.45f, -.75f), new Vector2(.6f, -1), new Vector2(.6f, -.75f), new Vector2(0, -1), new Vector2(.6f, 0)};
				// &
				points[38] = new Vector2[] {new Vector2(.2f, -.5f), new Vector2(.2f, 0), new Vector2(0, -.5f), new Vector2(0, -1f), new Vector2(.6f, -1f), new Vector2(0, -1f), new Vector2(.2f, -.5f), new Vector2(.6f, -1f), new Vector2(.6f, -1f), new Vector2(.6f, -.7f), new Vector2(.2f, 0), new Vector2(.5f, 0), new Vector2(.5f, -.5f), new Vector2(.5f, 0), new Vector2(0, -.5f), new Vector2(.5f, -.5f)};
				// '
				points[39] = new Vector2[] {new Vector2(.3f, -.25f), new Vector2(.45f, 0)};
				// (
				points[40] = new Vector2[] {new Vector2(.45f, 0), new Vector2(.15f, -.25f), new Vector2(.15f, -.25f), new Vector2(.15f, -.75f), new Vector2(.45f, -1), new Vector2(.15f, -.75f)};
				// )
				points[41] = new Vector2[] {new Vector2(.15f, 0), new Vector2(.45f, -.25f), new Vector2(.45f, -.25f), new Vector2(.45f, -.75f), new Vector2(.15f, -1), new Vector2(.45f, -.75f)};
				// *
				points[42] = new Vector2[] {new Vector2(.3f, -1f), new Vector2(.3f, 0), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.5f, -.1f), new Vector2(.1f, -.9f), new Vector2(.5f, -.9f), new Vector2(.1f, -.1f)};
				// +
				points[43] = new Vector2[] {new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(.3f, -.9f), new Vector2(.3f, -.1f)};
				// ,
				points[44] = new Vector2[] {new Vector2(0, -1), new Vector2(.15f, -.75f)};
				// -
				points[45] = new Vector2[] {new Vector2(0, -.5f), new Vector2(.6f, -.5f)};
				// .
				points[46] = new Vector2[] {new Vector2(0, -.9f), new Vector2(0, -1)};
				// /
				points[47] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, -1)};
				// 0
				points[48] = new Vector2[] {new Vector2(0, -1), new Vector2(0, 0), new Vector2(.6f, -1), new Vector2(.6f, 0), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(.6f, -1), new Vector2(0, -1)};
				// 1
				points[49] = new Vector2[] {new Vector2(.3f, -1), new Vector2(.3f, 0)};
				// 2
				points[50] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -1)};
				// 3
				points[51] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(.6f, -1), new Vector2(.6f, 0), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -.5f), new Vector2(.6f, -.5f)};
				// 4
				points[52] = new Vector2[] {new Vector2(0, -.5f), new Vector2(0, 0), new Vector2(.6f, -1), new Vector2(.6f, 0), new Vector2(.6f, -.5f), new Vector2(0, -.5f)};
				// 5
				points[53] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(0, -1)};
				// 6
				points[54] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(0, 0), new Vector2(0, -1)};
				// 7
				points[55] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(.6f, 0)};
				// 8
				points[56] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, -.5f), new Vector2(0, -.5f)};
				// 9
				points[57] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, 0), new Vector2(0, -.5f)};
				// :
				points[58] = new Vector2[] {new Vector2(0, -.9f), new Vector2(0, -1), new Vector2(0, -.3f), new Vector2(0, -.4f)};
				// ;
				points[59] = new Vector2[] {new Vector2(0, -1), new Vector2(.15f, -.75f), new Vector2(.1f, -.3f), new Vector2(.1f, -.4f)};
				// <
				points[60] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(0, -.5f)};
				// =
				points[61] = new Vector2[] {new Vector2(.6f, -.25f), new Vector2(0, -.25f), new Vector2(.6f, -.75f), new Vector2(0, -.75f)};
				// >
				points[62] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, -.5f), new Vector2(0, -1f), new Vector2(.6f, -.5f)};
				// ?
				points[63] = new Vector2[] {new Vector2(0, -.9f), new Vector2(0, -1), new Vector2(0, -.75f), new Vector2(0, -.5f), new Vector2(0, -.5f), new Vector2(.3f, -.5f), new Vector2(.3f, 0), new Vector2(.3f, -.5f), new Vector2(0, 0), new Vector2(.3f, 0)};
				// A 
				points[65] = new Vector2[] {new Vector2(0, -1), new Vector2(0, -.3f), new Vector2(.6f, -.3f), new Vector2(.6f, -1), new Vector2(.3f, 0), new Vector2(0, -.3f), new Vector2(.3f, 0), new Vector2(.6f, -.3f), new Vector2(0, -.5f), new Vector2(.6f, -.5f)};
				// B
				points[66] = new Vector2[] {new Vector2(0, -1), new Vector2(0, 0), new Vector2(.447f, 0), new Vector2(0, 0), new Vector2(.447f, 0), new Vector2(.6f, -.155f), new Vector2(.6f, -.347f), new Vector2(.6f, -.155f), new Vector2(.448f, -.5f), new Vector2(.6f, -.347f), new Vector2(.448f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -.653f), new Vector2(.448f, -.5f), new Vector2(.6f, -.653f), new Vector2(.6f, -.845f), new Vector2(.447f, -1), new Vector2(.6f, -.845f), new Vector2(0, -1), new Vector2(.447f, -1)};
				// C
				points[67] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -1)};	
				// D
				points[68] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.447f, 0), new Vector2(0, 0), new Vector2(.447f, 0), new Vector2(.6f, -.155f), new Vector2(.6f, -.845f), new Vector2(.6f, -.155f), new Vector2(.6f, -.845f), new Vector2(.447f, -1), new Vector2(.447f, -1), new Vector2(0, -1)};
				// E
				points[69] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.3f, -.5f)};
				// F
				points[70] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.3f, -.5f)};
				// G
				points[71] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -1), new Vector2(0, 0), new Vector2(.6f, -1), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.3f, -.5f), new Vector2(.6f, -.5f)};
				// H
				points[72] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(0, -.5f), new Vector2(.6f, -.5f)};
				// I
				points[73] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.3f, -1), new Vector2(.3f, 0)};
				// J
				points[74] = new Vector2[] {new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(.6f, 0), new Vector2(0, -1), new Vector2(0, -.725f)};
				// K
				points[75] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, 0), new Vector2(0, -.5f), new Vector2(.6f, -1)};
				// L
				points[76] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -1)};
				// M
				points[77] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(0, 0), new Vector2(.3f, -.5f), new Vector2(.6f, 0), new Vector2(.3f, -.5f), new Vector2(.6f, 0), new Vector2(.6f, -1)};
				// N
				points[78] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(.6f, -1), new Vector2(0, 0)};
				// O
				points[79] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(0, 0)};
				// P
				points[80] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(.6f, 0), new Vector2(.6f, -.5f)};
				// Q
				points[81] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(.6f, -1), new Vector2(.3f, -.5f)};
				// R
				points[82] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(.6f, 0), new Vector2(.6f, -.5f), new Vector2(.15f, -.5f), new Vector2(.6f, -1)};
				// S
				points[83] = new Vector2[] {new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(0, -1)};
				// T
				points[84] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(.3f, -1), new Vector2(.3f, 0)};
				// U
				points[85] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(.6f, 0), new Vector2(.6f, -1), new Vector2(.6f, -1), new Vector2(0, -1)};
				// V
				points[86] = new Vector2[] {new Vector2(.3f, -1), new Vector2(0, 0), new Vector2(.3f, -1), new Vector2(.6f, 0)};
				// W
				points[87] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(0, -1), new Vector2(.3f, -.5f), new Vector2(.6f, -1), new Vector2(.3f, -.5f), new Vector2(.6f, 0), new Vector2(.6f, -1)};
				// X
				points[88] = new Vector2[] {new Vector2(.6f, -1), new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(0, -1)};
				// Y
				points[89] = new Vector2[] {new Vector2(0, 0), new Vector2(.3f, -.5f), new Vector2(.6f, 0), new Vector2(.3f, -.5f), new Vector2(.3f, -1), new Vector2(.3f, -.5f)};
				// Z
				points[90] = new Vector2[] {new Vector2(.6f, 0), new Vector2(0, 0), new Vector2(.6f, 0), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(0, -1)};
				// [
				points[91] = new Vector2[] {new Vector2(.4f, 0), new Vector2(.1f, 0), new Vector2(.1f, -1f), new Vector2(.4f, -1f), new Vector2(.1f, -1f), new Vector2(.1f, 0)};
				// \
				points[92] = new Vector2[] {new Vector2(.6f, -1), new Vector2(0, 0)};
				// ]
				points[93] = new Vector2[] {new Vector2(.2f, 0), new Vector2(.5f, 0), new Vector2(.2f, -1f), new Vector2(.5f, -1f), new Vector2(.5f, 0), new Vector2(.5f, -1f)};
				// ^
				points[94] = new Vector2[] {new Vector2(0, -.5f), new Vector2(.3f, 0), new Vector2(.6f, -.5f), new Vector2(.3f, 0)};
				// _
				points[95] = new Vector2[] {new Vector2(0, -1f), new Vector2(.8f, -1f)};
				// `
				points[96] = new Vector2[] {new Vector2(.5f, -.3f), new Vector2(.3f, 0)};
				// a
				points[97] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, -.75f), new Vector2(0, -1), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(0, -.75f), new Vector2(.6f, -.75f), new Vector2(.6f, -1), new Vector2(0, -1)};
				// b
				points[98] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(.6f, -1), new Vector2(.6f, -.5f)};
				// c
				points[99] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(0, -1)};
				// d
				points[100] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(.6f, 0)};
				// e
				points[101] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(.6f, -.75f), new Vector2(0, -.75f), new Vector2(.6f, -.75f), new Vector2(.6f, -1), new Vector2(0, -1)};
				// f
				points[102] = new Vector2[] {new Vector2(.15f, -1), new Vector2(.15f, -.25f), new Vector2(.45f, 0), new Vector2(.3f, 0), new Vector2(.15f, -.25f), new Vector2(.3f, 0), new Vector2(.45f, -.5f), new Vector2(.15f, -.5f)};
				// g
				points[103] = new Vector2[] {new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(0, -1.25f), new Vector2(.6f, -1.25f), new Vector2(.6f, -1.25f), new Vector2(.6f, -.5f), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(0, -1), new Vector2(.6f, -1)};
				// h
				points[104] = new Vector2[] {new Vector2(0, 0), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(.6f, -.5f)};
				// i
				points[105] = new Vector2[] {new Vector2(.3f, -1), new Vector2(.3f, -.5f), new Vector2(.3f, -.25f), new Vector2(.3f, -.15f)};
				// j
				points[106] = new Vector2[] {new Vector2(.3f, -.25f), new Vector2(.3f, -.15f), new Vector2(.3f, -1.25f), new Vector2(.3f, -.5f), new Vector2(0, -1.25f), new Vector2(.3f, -1.25f)};
				// k
				points[107] = new Vector2[] {new Vector2(0, -1), new Vector2(0, 0), new Vector2(0, -.75f), new Vector2(.3f, -.5f), new Vector2(0, -.75f), new Vector2(.6f, -1)};
				// l
				points[108] = new Vector2[] {new Vector2(.3f, -1), new Vector2(.3f, 0)};
				// m
				points[109] = new Vector2[] {new Vector2(.45f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -.75f), new Vector2(.45f, -.5f), new Vector2(.6f, -.75f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.3f, -1), new Vector2(.3f, -.5f)};
				// n
				points[110] = new Vector2[] {new Vector2(.45f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -.75f), new Vector2(.45f, -.5f), new Vector2(.6f, -.75f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(0, -.5f)};
				// o
				points[111] = new Vector2[] {new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(.6f, -.5f)};
				// p
				points[112] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(.6f, -1), new Vector2(.6f, -.5f), new Vector2(0, -1.25f), new Vector2(0, -.5f)};
				// q
				points[113] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(0, -1), new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -1.25f), new Vector2(.6f, -.5f)};
				// r
				points[114] = new Vector2[] {new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(.6f, -.75f), new Vector2(.6f, -.5f)};
				// s
				points[115] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, -.75f), new Vector2(0, -.5f), new Vector2(.6f, -.75f), new Vector2(0, -.75f), new Vector2(.6f, -.75f), new Vector2(.6f, -1), new Vector2(.6f, -1), new Vector2(0, -1)};
				// t
				points[116] = new Vector2[] {new Vector2(.3f, -1), new Vector2(.3f, -.25f), new Vector2(.45f, -.5f), new Vector2(.15f, -.5f), new Vector2(.3f, -1), new Vector2(.45f, -1)};
				// u
				points[117] = new Vector2[] {new Vector2(0, -1), new Vector2(0, -.5f), new Vector2(.6f, -1), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(0, -1)};
				// v
				points[118] = new Vector2[] {new Vector2(.3f, -1), new Vector2(0, -.5f), new Vector2(.3f, -1), new Vector2(.6f, -.5f)};
				// w
				points[119] = new Vector2[] {new Vector2(.15f, -1), new Vector2(0, -.5f), new Vector2(.3f, -.75f), new Vector2(.15f, -1), new Vector2(.3f, -.75f), new Vector2(.45f, -1), new Vector2(.45f, -1), new Vector2(.6f, -.5f)};
				// x
				points[120] = new Vector2[] {new Vector2(.6f, -1), new Vector2(0, -.5f), new Vector2(0, -1), new Vector2(.6f, -.5f)};
				// y
				points[121] = new Vector2[] {new Vector2(0, -1.25f), new Vector2(.6f, -.5f), new Vector2(.3f, -.875f), new Vector2(0, -.5f)};
				// z
				points[122] = new Vector2[] {new Vector2(.6f, -.5f), new Vector2(0, -.5f), new Vector2(0, -1), new Vector2(.6f, -.5f), new Vector2(.6f, -1), new Vector2(0, -1)};
			}
			
			return points;
		}
	}
}

}