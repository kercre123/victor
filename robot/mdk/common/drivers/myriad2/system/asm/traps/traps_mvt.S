#include <swcLeonUtilsDefines.h>
#include <registersMyriad2.h>
#include <DrvIcbDefines.h>



! trap handling (multi vector trapping) 
                .section ".sys.text.traps", "ax", @progbits
                .type    traphandler, #function
                .global   traphandler
                .align    4
                
! tt = 0                
traphandler:
       mov 0, %l0
       unimp 0x00
       nop
       nop
trap_001:
       mov 1, %l0
       unimp 0x01
       nop
       nop
trap_002:
       mov 2, %l0
       unimp 0x02
       nop
       nop
trap_003:
       mov 3, %l0
       unimp 0x03
       nop
       nop
trap_004:
       mov 4, %l0
       unimp 0x04
       nop
       nop
trap_005:
       mov %psr, %g7
       sethi %hi(window_overflow_handler), %l4
       jmp %l4 + %lo(window_overflow_handler)
       rd  %tbr, %g5                
trap_006:
       mov %psr, %g7
       sethi %hi(window_underflow_handler), %l4
       jmp %l4 + %lo(window_underflow_handler)
       rd  %tbr, %g5                
trap_007:
       mov 7, %l0
       unimp 0x07
       nop
       nop
trap_008:
       mov 8, %l0
       unimp 0x08
       nop
       nop
trap_009:
       mov 9, %l0
       unimp 0x09
       nop
       nop
trap_010:
       mov 10, %l0
       unimp 0x0a
       nop
       nop
       
! external IRQ signals 
trap_011:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_012:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_013:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_014:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_015:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_016:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_017:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_018:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_019:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_020:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_021:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_022:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_023:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_024:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_025:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_026:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_027:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_028:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_029:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_030:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_031:
       mov %psr, %g7
       sethi %hi(ext_trap_handler), %l4
       jmp %l4 + %lo(ext_trap_handler)
       rd  %tbr, %g5                
trap_032:
       mov 32, %l0
       unimp 0x20
       nop
       nop
trap_033:
       mov 33, %l0
       unimp 0x21
       nop
       nop
trap_034:
       mov 34, %l0
       unimp 0x22
       nop
       nop
trap_035:
       mov 35, %l0
       unimp 0x23
       nop
       nop
trap_036:
       mov 36, %l0
       unimp 0x24
       nop
       nop
trap_037:
       mov 37, %l0
       unimp 0x25
       nop
       nop
trap_038:
       mov 38, %l0
       unimp 0x26
       nop
       nop
trap_039:
       mov 39, %l0
       unimp 0x27
       nop
       nop
trap_040:
       mov 40, %l0
       unimp 0x28
       nop
       nop
trap_041:
       mov 41, %l0
       unimp 0x29
       nop
       nop
trap_042:
       mov 42, %l0
       unimp 0x2a
       nop
       nop
trap_043:
       mov 43, %l0
       unimp 0x2b
       nop
       nop
trap_044:
       mov 44, %l0
       unimp 0x2c
       nop
       nop
trap_045:
       mov 45, %l0
       unimp 0x2d
       nop
       nop
trap_046:
       mov 46, %l0
       unimp 0x2e
       nop
       nop
trap_047:
       mov 47, %l0
       unimp 0x2f
       nop
       nop
trap_048:
       mov 48, %l0
       unimp 0x30
       nop
       nop
trap_049:
       mov 49, %l0
       unimp 0x31
       nop
       nop
trap_050:
       mov 50, %l0
       unimp 0x32
       nop
       nop
trap_051:
       mov 51, %l0
       unimp 0x33
       nop
       nop
trap_052:
       mov 52, %l0
       unimp 0x34
       nop
       nop
trap_053:
       mov 53, %l0
       unimp 0x35
       nop
       nop
trap_054:
       mov 54, %l0
       unimp 0x36
       nop
       nop
trap_055:
       mov 55, %l0
       unimp 0x37
       nop
       nop
trap_056:
       mov 56, %l0
       unimp 0x38
       nop
       nop
trap_057:
       mov 57, %l0
       unimp 0x39
       nop
       nop
trap_058:
       mov 58, %l0
       unimp 0x3a
       nop
       nop
trap_059:
       mov 59, %l0
       unimp 0x3b
       nop
       nop
trap_060:
       mov 60, %l0
       unimp 0x3c
       nop
       nop
trap_061:
       mov 61, %l0
       unimp 0x3d
       nop
       nop
trap_062:
       mov 62, %l0
       unimp 0x3e
       nop
       nop
trap_063:
       mov 63, %l0
       unimp 0x3f
       nop
       nop
trap_064:
       mov 64, %l0
       unimp 0x40
       nop
       nop
trap_065:
       mov 65, %l0
       unimp 0x41
       nop
       nop
trap_066:
       mov 66, %l0
       unimp 0x42
       nop
       nop
trap_067:
       mov 67, %l0
       unimp 0x43
       nop
       nop
trap_068:
       mov 68, %l0
       unimp 0x44
       nop
       nop
trap_069:
       mov 69, %l0
       unimp 0x45
       nop
       nop
trap_070:
       mov 70, %l0
       unimp 0x46
       nop
       nop
trap_071:
       mov 71, %l0
       unimp 0x47
       nop
       nop
trap_072:
       mov 72, %l0
       unimp 0x48
       nop
       nop
trap_073:
       mov 73, %l0
       unimp 0x49
       nop
       nop
trap_074:
       mov 74, %l0
       unimp 0x4a
       nop
       nop
trap_075:
       mov 75, %l0
       unimp 0x4b
       nop
       nop
trap_076:
       mov 76, %l0
       unimp 0x4c
       nop
       nop
trap_077:
       mov 77, %l0
       unimp 0x4d
       nop
       nop
trap_078:
       mov 78, %l0
       unimp 0x4e
       nop
       nop
trap_079:
       mov 79, %l0
       unimp 0x4f
       nop
       nop
trap_080:
       mov 80, %l0
       unimp 0x50
       nop
       nop
trap_081:
       mov 81, %l0
       unimp 0x51
       nop
       nop
trap_082:
       mov 82, %l0
       unimp 0x52
       nop
       nop
trap_083:
       mov 83, %l0
       unimp 0x53
       nop
       nop
trap_084:
       mov 84, %l0
       unimp 0x54
       nop
       nop
trap_085:
       mov 85, %l0
       unimp 0x55
       nop
       nop
trap_086:
       mov 86, %l0
       unimp 0x56
       nop
       nop
trap_087:
       mov 87, %l0
       unimp 0x57
       nop
       nop
trap_088:
       mov 88, %l0
       unimp 0x58
       nop
       nop
trap_089:
       mov 89, %l0
       unimp 0x59
       nop
       nop
trap_090:
       mov 90, %l0
       unimp 0x5a
       nop
       nop
trap_091:
       mov 91, %l0
       unimp 0x5b
       nop
       nop
trap_092:
       mov 92, %l0
       unimp 0x5c
       nop
       nop
trap_093:
       mov 93, %l0
       unimp 0x5d
       nop
       nop
trap_094:
       mov 94, %l0
       unimp 0x5e
       nop
       nop
trap_095:
       mov 95, %l0
       unimp 0x5f
       nop
       nop
trap_096:
       mov 96, %l0
       unimp 0x60
       nop
       nop
trap_097:
       mov 97, %l0
       unimp 0x61
       nop
       nop
trap_098:
       mov 98, %l0
       unimp 0x62
       nop
       nop
trap_099:
       mov 99, %l0
       unimp 0x63
       nop
       nop
trap_100:
       mov 100, %l0
       unimp 0x64
       nop
       nop
trap_101:
       mov 101, %l0
       unimp 0x65
       nop
       nop
trap_102:
       mov 102, %l0
       unimp 0x66
       nop
       nop
trap_103:
       mov 103, %l0
       unimp 0x67
       nop
       nop
trap_104:
       mov 104, %l0
       unimp 0x68
       nop
       nop
trap_105:
       mov 105, %l0
       unimp 0x69
       nop
       nop
trap_106:
       mov 106, %l0
       unimp 0x6a
       nop
       nop
trap_107:
       mov 107, %l0
       unimp 0x6b
       nop
       nop
trap_108:
       mov 108, %l0
       unimp 0x6c
       nop
       nop
trap_109:
       mov 109, %l0
       unimp 0x6d
       nop
       nop
trap_110:
       mov 110, %l0
       unimp 0x6e
       nop
       nop
trap_111:
       mov 111, %l0
       unimp 0x6f
       nop
       nop
trap_112:
       mov 112, %l0
       unimp 0x70
       nop
       nop
trap_113:
       mov 113, %l0
       unimp 0x71
       nop
       nop
trap_114:
       mov 114, %l0
       unimp 0x72
       nop
       nop
trap_115:
       mov 115, %l0
       unimp 0x73
       nop
       nop
trap_116:
       mov 116, %l0
       unimp 0x74
       nop
       nop
trap_117:
       mov 117, %l0
       unimp 0x75
       nop
       nop
trap_118:
       mov 118, %l0
       unimp 0x76
       nop
       nop
trap_119:
       mov 119, %l0
       unimp 0x77
       nop
       nop
trap_120:
       mov 120, %l0
       unimp 0x78
       nop
       nop
trap_121:
       mov 121, %l0
       unimp 0x79
       nop
       nop
trap_122:
       mov 122, %l0
       unimp 0x7a
       nop
       nop
trap_123:
       mov 123, %l0
       unimp 0x7b
       nop
       nop
trap_124:
       mov 124, %l0
       unimp 0x7c
       nop
       nop
trap_125:
       mov 125, %l0
       unimp 0x7d
       nop
       nop
trap_126:
       mov 126, %l0
       unimp 0x7e
       nop
       nop
trap_127:
       mov 127, %l0
       unimp 0x7f
       nop
       nop
trap_128:
       mov 128, %l0
       unimp 0x80
       nop
       nop
trap_129:
       mov 129, %l0
       unimp 0x81
       nop
       nop
trap_130:
       mov 130, %l0
       unimp 0x82
       nop
       nop
trap_131:
       mov 131, %l0
       unimp 0x83
       nop
       nop
trap_132:
       mov 132, %l0
       unimp 0x84
       nop
       nop
trap_133:
       mov 133, %l0
       unimp 0x85
       nop
       nop
trap_134:
       mov 134, %l0
       unimp 0x86
       nop
       nop
trap_135:
       mov 135, %l0
       unimp 0x87
       nop
       nop
trap_136:
       mov 136, %l0
       unimp 0x88
       nop
       nop
trap_137:
       mov 137, %l0
       unimp 0x89
       nop
       nop
trap_138:
       mov 138, %l0
       unimp 0x8a
       nop
       nop
trap_139:
       mov 139, %l0
       unimp 0x8b
       nop
       nop
trap_140:
       mov 140, %l0
       unimp 0x8c
       nop
       nop
trap_141:
       mov 141, %l0
       unimp 0x8d
       nop
       nop
trap_142:
       mov 142, %l0
       unimp 0x8e
       nop
       nop
trap_143:
       mov 143, %l0
       unimp 0x8f
       nop
       nop
trap_144:
       mov 144, %l0
       unimp 0x90
       nop
       nop
trap_145:
       mov 145, %l0
       unimp 0x91
       nop
       nop
trap_146:
       mov 146, %l0
       unimp 0x92
       nop
       nop
trap_147:
       mov 147, %l0
       unimp 0x93
       nop
       nop
trap_148:
       mov 148, %l0
       unimp 0x94
       nop
       nop
trap_149:
       mov 149, %l0
       unimp 0x95
       nop
       nop
trap_150:
       mov 150, %l0
       unimp 0x96
       nop
       nop
trap_151:
       mov 151, %l0
       unimp 0x97
       nop
       nop
trap_152:
       mov 152, %l0
       unimp 0x98
       nop
       nop
trap_153:
       mov 153, %l0
       unimp 0x99
       nop
       nop
trap_154:
       mov 154, %l0
       unimp 0x9a
       nop
       nop
trap_155:
       mov 155, %l0
       unimp 0x9b
       nop
       nop
trap_156:
       mov 156, %l0
       unimp 0x9c
       nop
       nop
trap_157:
       mov 157, %l0
       unimp 0x9d
       nop
       nop
trap_158:
       mov 158, %l0
       unimp 0x9e
       nop
       nop
trap_159:
       mov 159, %l0
       unimp 0x9f
       nop
       nop
trap_160:
       mov 160, %l0
       unimp 0xa0
       nop
       nop
trap_161:
       mov 161, %l0
       unimp 0xa1
       nop
       nop
trap_162:
       mov 162, %l0
       unimp 0xa2
       nop
       nop
trap_163:
       mov 163, %l0
       unimp 0xa3
       nop
       nop
trap_164:
       mov 164, %l0
       unimp 0xa4
       nop
       nop
trap_165:
       mov 165, %l0
       unimp 0xa5
       nop
       nop
trap_166:
       mov 166, %l0
       unimp 0xa6
       nop
       nop
trap_167:
       mov 167, %l0
       unimp 0xa7
       nop
       nop
trap_168:
       mov 168, %l0
       unimp 0xa8
       nop
       nop
trap_169:
       mov 169, %l0
       unimp 0xa9
       nop
       nop
trap_170:
       mov 170, %l0
       unimp 0xaa
       nop
       nop
trap_171:
       mov 171, %l0
       unimp 0xab
       nop
       nop
trap_172:
       mov 172, %l0
       unimp 0xac
       nop
       nop
trap_173:
       mov 173, %l0
       unimp 0xad
       nop
       nop
trap_174:
       mov 174, %l0
       unimp 0xae
       nop
       nop
trap_175:
       mov 175, %l0
       unimp 0xaf
       nop
       nop
trap_176:
       mov 176, %l0
       unimp 0xb0
       nop
       nop
trap_177:
       mov 177, %l0
       unimp 0xb1
       nop
       nop
trap_178:
       mov 178, %l0
       unimp 0xb2
       nop
       nop
trap_179:
       mov 179, %l0
       unimp 0xb3
       nop
       nop
trap_180:
       mov 180, %l0
       unimp 0xb4
       nop
       nop
trap_181:
       mov 181, %l0
       unimp 0xb5
       nop
       nop
trap_182:
       mov 182, %l0
       unimp 0xb6
       nop
       nop
trap_183:
       mov 183, %l0
       unimp 0xb7
       nop
       nop
trap_184:
       mov 184, %l0
       unimp 0xb8
       nop
       nop
trap_185:
       mov 185, %l0
       unimp 0xb9
       nop
       nop
trap_186:
       mov 186, %l0
       unimp 0xba
       nop
       nop
trap_187:
       mov 187, %l0
       unimp 0xbb
       nop
       nop
trap_188:
       mov 188, %l0
       unimp 0xbc
       nop
       nop
trap_189:
       mov 189, %l0
       unimp 0xbd
       nop
       nop
trap_190:
       mov 190, %l0
       unimp 0xbe
       nop
       nop
trap_191:
       mov 191, %l0
       unimp 0xbf
       nop
       nop
trap_192:
       mov 192, %l0
       unimp 0xc0
       nop
       nop
trap_193:
       mov 193, %l0
       unimp 0xc1
       nop
       nop
trap_194:
       mov 194, %l0
       unimp 0xc2
       nop
       nop
trap_195:
       mov 195, %l0
       unimp 0xc3
       nop
       nop
trap_196:
       mov 196, %l0
       unimp 0xc4
       nop
       nop
trap_197:
       mov 197, %l0
       unimp 0xc5
       nop
       nop
trap_198:
       mov 198, %l0
       unimp 0xc6
       nop
       nop
trap_199:
       mov 199, %l0
       unimp 0xc7
       nop
       nop
trap_200:
       mov 200, %l0
       unimp 0xc8
       nop
       nop
trap_201:
       mov 201, %l0
       unimp 0xc9
       nop
       nop
trap_202:
       mov 202, %l0
       unimp 0xca
       nop
       nop
trap_203:
       mov 203, %l0
       unimp 0xcb
       nop
       nop
trap_204:
       mov 204, %l0
       unimp 0xcc
       nop
       nop
trap_205:
       mov 205, %l0
       unimp 0xcd
       nop
       nop
trap_206:
       mov 206, %l0
       unimp 0xce
       nop
       nop
trap_207:
       mov 207, %l0
       unimp 0xcf
       nop
       nop
trap_208:
       mov 208, %l0
       unimp 0xd0
       nop
       nop
trap_209:
       mov 209, %l0
       unimp 0xd1
       nop
       nop
trap_210:
       mov 210, %l0
       unimp 0xd2
       nop
       nop
trap_211:
       mov 211, %l0
       unimp 0xd3
       nop
       nop
trap_212:
       mov 212, %l0
       unimp 0xd4
       nop
       nop
trap_213:
       mov 213, %l0
       unimp 0xd5
       nop
       nop
trap_214:
       mov 214, %l0
       unimp 0xd6
       nop
       nop
trap_215:
       mov 215, %l0
       unimp 0xd7
       nop
       nop
trap_216:
       mov 216, %l0
       unimp 0xd8
       nop
       nop
trap_217:
       mov 217, %l0
       unimp 0xd9
       nop
       nop
trap_218:
       mov 218, %l0
       unimp 0xda
       nop
       nop
trap_219:
       mov 219, %l0
       unimp 0xdb
       nop
       nop
trap_220:
       mov 220, %l0
       unimp 0xdc
       nop
       nop
trap_221:
       mov 221, %l0
       unimp 0xdd
       nop
       nop
trap_222:
       mov 222, %l0
       unimp 0xde
       nop
       nop
trap_223:
       mov 223, %l0
       unimp 0xdf
       nop
       nop
trap_224:
       mov 224, %l0
       unimp 0xe0
       nop
       nop
trap_225:
       mov 225, %l0
       unimp 0xe1
       nop
       nop
trap_226:
       mov 226, %l0
       unimp 0xe2
       nop
       nop
trap_227:
       mov 227, %l0
       unimp 0xe3
       nop
       nop
trap_228:
       mov 228, %l0
       unimp 0xe4
       nop
       nop
trap_229:
       mov 229, %l0
       unimp 0xe5
       nop
       nop
trap_230:
       mov 230, %l0
       unimp 0xe6
       nop
       nop
trap_231:
       mov 231, %l0
       unimp 0xe7
       nop
       nop
trap_232:
       mov 232, %l0
       unimp 0xe8
       nop
       nop
trap_233:
       mov 233, %l0
       unimp 0xe9
       nop
       nop
trap_234:
       mov 234, %l0
       unimp 0xea
       nop
       nop
trap_235:
       mov 235, %l0
       unimp 0xeb
       nop
       nop
trap_236:
       mov 236, %l0
       unimp 0xec
       nop
       nop
trap_237:
       mov 237, %l0
       unimp 0xed
       nop
       nop
trap_238:
       mov 238, %l0
       unimp 0xee
       nop
       nop
trap_239:
       mov 239, %l0
       unimp 0xef
       nop
       nop
trap_240:
       mov 240, %l0
       unimp 0xf0
       nop
       nop
trap_241:
       mov 241, %l0
       unimp 0xf1
       nop
       nop
trap_242:
       mov 242, %l0
       unimp 0xf2
       nop
       nop
trap_243:
       mov 243, %l0
       unimp 0xf3
       nop
       nop
trap_244:
       mov 244, %l0
       unimp 0xf4
       nop
       nop
trap_245:
       mov 245, %l0
       unimp 0xf5
       nop
       nop
trap_246:
       mov 246, %l0
       unimp 0xf6
       nop
       nop
trap_247:
       mov 247, %l0
       unimp 0xf7
       nop
       nop
trap_248:
       mov 248, %l0
       unimp 0xf8
       nop
       nop
trap_249:
       mov 249, %l0
       unimp 0xf9
       nop
       nop
trap_250:
       mov 250, %l0
       unimp 0xfa
       nop
       nop
trap_251:
       mov 251, %l0
       unimp 0xfb
       nop
       nop
trap_252:
       mov 252, %l0
       unimp 0xfc
       nop
       nop
trap_253:
       mov 253, %l0
       unimp 0xfd
       nop
       nop
trap_254:
       mov 254, %l0
       unimp 0xfe
       nop
       nop
trap_255:
       mov 255, %l0
       unimp 0xff
       nop
       nop

       
!;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                !; restore current registers window from stack 
                !; return address: %g1 
.L.restore_regs:
                  sll        %g6, 1, %g5
                  srl        %g6, 7, %g6
                  bset        %g5, %g6
                !; get into the window to restore (2 above) 
                restore
                restore
                wr            %g6, %wim
                !; restore its contents  
                ldd        [ %sp + 0x00 ], %l0
                ldd        [ %sp + 0x08 ], %l2
                ldd        [ %sp + 0x10 ], %l4
                ldd        [ %sp + 0x18 ], %l6
                ldd        [ %sp + 0x20 ], %i0
                ldd        [ %sp + 0x28 ], %i2
                ldd        [ %sp + 0x30 ], %i4
                ldd        [ %sp + 0x38 ], %i6
                !; return to, aaa, the caller (perform only ONE save, TWO are needed)
                jmp     %g1 + 8 
                save       
                
               !; save the next window on the stack 
                !; return address: %g1
                !; destroys: %g5; updates: %g6
.L.save_regs:
                !; rotate WIM to the right (assume %wim has just been cleared) 
                srl        %g6, 1, %g5
                sll     %g6, 7, %g6
                bset   %g5, %g6
                !; get into the window to save (1 below) 
                save
                wr      %g6, %wim
                !; save its contents 
                std        %l0, [ %sp + 0x00 ]
                std        %l2, [ %sp + 0x08 ]
                std        %l4, [ %sp + 0x10 ]
                std        %l6, [ %sp + 0x18 ]
                std        %i0, [ %sp + 0x20 ]
                std        %i2, [ %sp + 0x28 ]
                std        %i4, [ %sp + 0x30 ]
                std        %i6, [ %sp + 0x38 ]
                !; return to, aaa, the caller 
                jmp     %g1 + 8
                restore                
       
       
! get out of interrupt in this case       
return_reexecute:
       mov   %g7, %psr 
       nop              ! flush new psr value trough pipeline, 3 nops  
       nop
       mov   %l5, %g1
       jmp %l1
       rett %l2
       
! =============================================================================       
! =============================================================================
! =============================================================================
! =================== entrance in the trap handler ========================================        
! =============================================================================
! =============================================================================
! =============================================================================
ext_trap_handler:
                .type   ext_trap_handler, #function
                
#ifdef BUILD_FOR_VCS
                ! save for VCS the tt
                set        LEON_TRAP_ADR, %g6
                st    %g5, [%g6]
#endif
                
                 rd      %wim, %g6
                !; extract trap ID in %l0
                and        %g5, MASK_TBR_tt, %l0
                mov     %g1, %l5
                
                rd %asr17,%l0        !; Read PCR Value
                srl %l0,28,%l0       !; %l0 = (PCR >> 28) (i.e. Processor index
                cmp %l0,1            !; Processor index 1 == LEON_RT; Processor Index 0 == LEON_OS
                be  .L.read_icb_source
                 sethi   %hi(ICB1_BASE_ADR), %l0  ;! Set Base for LeonRT (branch delay slot)
                sethi   %hi(ICB0_BASE_ADR), %l0   ;! Set Base for LeonOS
.L.read_icb_source:
                ld        [ %l0 + %lo(ICB0_SRC_ADR) ], %l3 ! %l3 = ICB.SRC
                !; get out if we can't tell which it was (level-triggered IRQ
                !; which somehow got cleared already)
                cmp     %l3, __N_IRQS__
                bgeu    return_reexecute
                !; disable the interrupt
                mov     1, %l4
                sll     %l4, %l3, %g1 !; %g1 = 1 <<( ICB.SRC & 0x1F )
                srl     %l3, 5, %l6
                sll     %l6, 2, %l6 !; %l6 =( ICB.SRC / 32 )* 4 = (ICB.SRC>=32)?4:0;
                add     %l0, %l6, %l0 
                !; clear the pending flag
                st      %g1, [ %l0 + %lo(ICB0_CLEAR_0_ADR) ] !; ICB.CLEAR[n] = 1 <<( ICB.SRC & 0x1F )
                !; read the handler pointer 
                sll        %l3, 2, %l6 !; %l3 = ICB.SRC << 2
                set        __irq_table, %l7
                ld        [ %l7 + %l6 ], %l6
                !; bail out if we don't have a handler 
                tst        %l6
                bz,a        __T_IRQ_no_handler
                ld      [ %l0 + %lo(ICB0_ENABLE_0_ADR) ], %l6

__T_IRQ_have_handler:
                !; see if we're in the invalid window 
                sll        %l4, %g7, %g1 ! l5 = 1<<psr.cwp
                cmp        %g1, %g6 ! in_wim
                !; and backup input PSR to local register %l7
                bne        __T_IRQ_crt_window_valid
                mov   %g7, %l7

__T_IRQ_crt_window_invalid:
                !; we are: we need to put the NEXT window on the stack 
                !;   so 
                !; perform the overflow logic here 
                sethi %hi( .L.save_regs ), %g1
                jmpl    %g1 + %lo( .L.save_regs ), %g1
                  wr    %g0, %wim
                !; at this stage, %g5 was destroyed, so re-read it
__T_IRQ_crt_window_valid:
                !; setup this window's stack 
                add        %fp, -64, %sp
                !; backup %y 
                rd      %y, %l0
#ifdef USE_INTERRUPT_PIL
!; set PIL to interrupt's IL 
                !; note, %g5 MIGHT have been destroyed by now - so re-read TBR
                rd        %tbr, %g5
                !; shift/mask the IL in TBR 
                and     %g5, 0xF0, %g5
                !; prepare PSR ( PIL<-IRQ.IL, ET=0 ) 
                andn    %l7, MASK_PSR_PIL, %g7
                sll     %g5, 4, %g5
                bset    %g5, %g7
#else
!; set PIL to max (0xF)
                !; prepare PSR ( PIL<-0xF, ET=0 ) 
                or      %l7, MASK_PSR_PIL, %g7
#endif
__T_IRQ_call_handler:
                !; setup the handler's argument (ICB.SRC)
                mov           %l3, %o0
                !; write PSR ( new PIL, ET=0 ) 
                wr        %g7, %psr
                !; write PSR ( new PIL, ET=1 ) 
                wr        %g7, PSR_ET, %psr
                mov        %g2, %l3 !; backup %g2
                mov        %g3, %l4 !; backup %g3

                !; call the handler - (*__irq_table[ICB.SRC])(ICB.SRC)
                call        %l6
                mov %l6,   %l3
                mov        %g4, %l6 !; backup %g4
                
__T_IRQ_done:
                !; reset %psr to what it was initially in the trap handler 
                !; (we're only doing this to disable traps)
                wr        %l7, %psr !; doesn't change CWP; ET = 0, PIL = original PIL
                 mov        %l6, %g4 !; restore %g4
                 add           %l7, 1, %g5 !; %g5&0x1f := psr.cwp+1
                 mov        %l4, %g3 !; restore %g3

                mov        1, %g7 !; %g7 = 1
                and     %g5, 0x7, %g5 !; %g5 = (1+psr.cwp)%8
                mov        %l3, %g2 !; restore %g2
                rd            %wim, %g6 ! in_wim !' re-read %wim - could have changed in ISR
                sll        %g7, %g5, %g7 ! %g7 = 1<<((psr.cwp+1)%8)
                wr            %l0, %y !; restore %y
                cmp        %g7, %g6 !; in_wim
                !; and restore backup input PSR from local register %l7
                bne        return_reexecute
                  mov   %l7, %g7 ! in_psr

__T_IRQ_prev_window_invalid:
                sethi   %hi( return_reexecute - 8 ), %g1
                !; we are: we need to get the PREVIOUS window from the stack
                !;   SO
                !; --cwp (we're fine, the cwp+1 one is invalid)
                !; (we need this because the restore_regs code does two restore's)
                save
                !; restore the window we're returning to
                bset    %lo( return_reexecute - 8 ), %g1
                b       .L.restore_regs
                  wr    %g0, %wim

__T_IRQ_no_handler:
                !; disable the interrupt source 
                bclr    %g1, %l6
                st      %l6, [ %l0 + %lo(ICB0_ENABLE_0_ADR) ]
                !; return, clearing the pending flag (again, just in case it's level triggered) 
                b       return_reexecute
                st    %g1, [ %l0 + %lo(ICB0_CLEAR_0_ADR) ]
       

! ===============================================================
window_overflow_handler:  


        set      LEON_TRAP_ADR, %g6     ! telling vcs what int was 
        st       %g5, [%g6]
              

 ! /* rotate WIM on bit right, we have 8 windows */
        mov %wim, %l3
        sll %l3,7,%l4
        srl %l3,1,%l3
        or  %l3,%l4,%l3
        and %l3,0xff,%l3

        !/* disable WIM traps */
        mov %g0,%wim
        nop 
        nop 
        nop

        !/* point to correct window */
        save

        !/* dump registers to stack */
        std %l0, [%sp +  0]
        std %l2, [%sp +  8]
        std %l4, [%sp + 16]
        std %l6, [%sp + 24]
        std %i0, [%sp + 32]
        std %i2, [%sp + 40]
        std %i4, [%sp + 48]
        std %i6, [%sp + 56]

        !/* back to where we should be */
        restore

        !/* set new value of window */
        mov %l3,%wim
        nop 
        nop
        nop

        !/* go home */
        jmp %l1
        rett %l2
  

! =================================================================================





window_underflow_handler:       
        set      LEON_TRAP_ADR, %g6     ! telling vcs what int was 
        st       %g5, [%g6]
       
      !/* rotate WIM on bit LEFT, we have 8 windows */ 
        mov %wim,%l3
        srl %l3,7,%l4
        sll %l3,1,%l3
        or  %l3,%l4,%l3
        and %l3,0xff,%l3

       ! /* disable WIM traps */
        mov %g0,%wim
        nop
        nop
        nop

        !/* point to correct window */
        restore
        restore

        !/* dump registers to stack */
        ldd [%sp +  0], %l0
        ldd [%sp +  8], %l2
        ldd [%sp + 16], %l4
        ldd [%sp + 24], %l6
        ldd [%sp + 32], %i0
        ldd [%sp + 40], %i2
        ldd [%sp + 48], %i4
        ldd [%sp + 56], %i6

        !/* back to where we should be */
        save
        save

        !/* set new value of window */
        mov %l3,%wim
        nop
        nop
        nop

        !/* go home */
        jmp %l1
        rett %l2       
       
       
       
       
       
      
       
               .size   traphandler, . - traphandler
               
               
!;============================================================================

		.section ".text.swcLeonSetPIL", "ax", @progbits
		.type   swcLeonSetPIL, #function
		.global swcLeonSetPIL
swcLeonSetPIL:
                !; input is in %o0, output in %o0 
                !; extract current PIL / form new PSR
                rd      %psr, %o1
                sll     %o0, POS_PSR_PIL, %o2 ! %o2 = new_pil<<PIL_POS
                and     %o1, MASK_PSR_PIL, %o0 ! %o0 = psr&PIL_MASK
                bclr    MASK_PSR_PIL, %o1 ! %o1 = psr&~PIL_MASK
                and     %o2, MASK_PSR_PIL, %o2 ! %o3 = (new_pil<<PIL_POS)&PIL_MASK
                wr      %o2, %o1, %psr
                 srl    %o0, POS_PSR_PIL, %o0 ! return the old PIL
                 retl
                  nop
                .size   swcLeonSetPIL, . - swcLeonSetPIL

!;============================================================================

		.section ".text.swcLeonDisableTraps", "ax", @progbits
		.type	swcLeonDisableTraps, #function
		.global swcLeonDisableTraps
swcLeonDisableTraps:
                rd      %psr, %o0
                andn    %o0, PSR_ET, %o1
                wr      %o1, %psr
                 and    %o0, PSR_ET, %o0
                 retl
                  srl   %o0, 5, %o0 ! return the old ET
                .size   swcLeonDisableTraps, . - swcLeonDisableTraps

!;============================================================================

		.section ".text.swcLeonEnableTraps", "ax", @progbits
		.type	swcLeonEnableTraps, #function
		.global swcLeonEnableTraps
swcLeonEnableTraps:
                rd      %psr, %o0
                or      %o0, PSR_ET, %o1
                wr      %o1, %psr
                 and    %o0, PSR_ET, %o0
                 retl
                  srl   %o0, 5, %o0 ! return the old ET
                .size   swcLeonEnableTraps, . - swcLeonEnableTraps

!;============================================================================

		.section ".text.swcLeonFlushWindows", "ax", @progbits
		.type	swcLeonFlushWindows, #function
		.global swcLeonFlushWindows
swcLeonFlushWindows:
                ! disable IRQs / enable traps
                rd      %psr, %g2
                or      %g2, MASK_PSR_PIL|PSR_ET, %g3
                wr      %g3, %psr
                 mov    %o7, %g4
                 and    %g2, MASK_PSR_CWP, %o0 ! return the CWP
                 nop
                save    %g0, 0, %sp
                save    %g0, 0, %sp
                save    %g0, 0, %sp
                save    %g0, 0, %sp
                save    %g0, 0, %sp
                save    %g0, 0, %sp
                wr      %g2, %psr
                 nop
                 jmp    %g4 + 8 ! retl %g4 ;)
                  nop
                .size   swcLeonFlushWindows, . - swcLeonFlushWindows
               
               