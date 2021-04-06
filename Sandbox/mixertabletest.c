#include <stdio.h>
#include <stdlib.h>

int mixer_table[] = { 0x0,        0x0,         0x0,         0x1000000,   0x1010101,   0x1010101, 0x101, 0x1010100,
0x2010101,  0x2020202,   0x20202,     0x1010100,   0x2020201,   0x3030202,
0x3030303,  0x1010000,   0x3020202,   0x4030303,   0x4040404,   0x1000004,
0x3020201,  0x4040403,   0x5050504,   0x1000505,   0x3020201,   0x5040403,
0x6060505,  0x60606,     0x3020101,   0x5040403,   0x6060605,   0x7070707,
0x2020100,  0x5040403,   0x7070606,   0x8080807,   0x2010008,   0x5040303,
0x7070606,  0x9090808,   0x1000909,   0x5040302,   0x8070606,   0x0A090908,
0x0A0A0A,   0x4030201,   0x8070605,   0x0A0A0909,  0x0B0B0B0B,  0x3020100,
0x8070605,  0x0B0A0908,  0x0C0C0B0B,  0x301000C,   0x7060504,
0x0B0A0908, 0x0D0C0C0B,  0x1000D0D,   0x7050403,   0x0B0A0908,
0x0D0D0C0C, 0x0E0E0E,    0x6040301,   0x0B0A0807,  0x0E0D0C0C,
0x0F0F0F0E, 0x5030200,   0x0A090806,  0x0E0D0C0B,  0x10100F0F,
0x10 };

int mixer_table_2[] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
						0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  1,  1,  1,  1,  2,  2,  2,
						2,  2,  3,  3,  3,  3,  3,  3,  0,  0,  1,  1,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  0,
						0,  1,  1,  2,  2,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,
						5,  5,  6,  6,  6,  6,  6,  0,  1,  1,  2,  3,  3,  4,  4,  5,  5,  6,  6,  6,  7,  7,  7,  7,  0,  1,
						2,  2,  3,  4,  4,  5,  6,  6,  7,  7,  7,  8,  8,  8,  8,  0,  1,  2,  3,  3,  4,  5,  6,  6,  7,  7,
						8,  8,  9,  9,  9,  9,  0,  1,  2,  3,  4,  5,  6,  6,  7,  8,  8,  9,  9,  10, 10, 10, 10, 0,  1,  2,
						3,  4,  5,  6,  7,  8,  9,  9,  10, 10, 11, 11, 11, 11, 0,  1,  2,  3,  5,  6,  7,  8,  8,  9,  10, 11,
						11, 11, 12, 12, 12, 0,  1,  3,  4,  5,  6,  7,  8,  9,  10, 11, 11, 12, 12, 13, 13, 13, 0,  1,  3,  4,
						5,  7,  8,  9,  10, 11, 12, 12, 13, 13, 14, 14, 14, 0,  1,  3,  4,  6,  7,  8,  10, 11, 12, 12, 13, 14, 
						14, 15, 15, 15, 0,  2,  3,  5,  6,  8,  9,  10, 11, 12, 13, 14, 15, 15, 16, 16, 16 };


int maina() {
	int volume = 0x7F;
	int pan = 64;
	int channelVolume = volume >> 3;
	if (volume)
		++channelVolume;
	if (channelVolume >= 17)
		channelVolume = 16;
	int channelPan = (pan / 8) - 8;
	if (pan > 64)
		++channelPan;

	int rightChannel = *((char *)mixer_table + 17 * channelVolume + channelPan);
	int leftChannel = *((char *)mixer_table + 17 * channelVolume - channelPan);

	int rightChannel_2 = mixer_table_2[17 * channelVolume + channelPan];
	int leftChannel_2 = mixer_table_2[17 * channelVolume - channelPan];

	printf("right: %d %d\n", rightChannel, rightChannel_2);
	printf("left: %d %d\n", leftChannel, leftChannel_2);
	printf("vol: %d, pan: %d\n", channelVolume, channelPan);

	for (int i = 0; i<128; i++) {
		for (int j = 0; j<128; j++) {
			channelVolume = i >> 3;
			if (i)
				++channelVolume;
			if (channelVolume >= 17)
				channelVolume = 16;
			channelPan = (j / 8) - 8;
			if (j > 64)
				++channelPan;
			channelPan = channelVolume == 0 ? 0 : channelPan;

			int rightChannel = *((char *)mixer_table + 17 * channelVolume + channelPan);
			int leftChannel = *((char *)mixer_table + 17 * channelVolume - channelPan);
			int rightChannel_2 = mixer_table_2[17 * channelVolume + channelPan];
			int leftChannel_2 = mixer_table_2[17 * channelVolume - channelPan];

			if (rightChannel != rightChannel_2) {
				printf("FAILURE vol %d pan %d right in position %d! %d != %d\n", channelVolume, channelPan, 17 * channelVolume + channelPan, rightChannel, rightChannel_2);

			}
			if (leftChannel != leftChannel_2) {
				printf("FAILURE vol %d pan %d left in position %d! %d != %d\n", channelVolume, channelPan, 17 * channelVolume - channelPan, leftChannel, leftChannel_2);
			}
			//printf("right: %d %d\n", rightChannel, rightChannel_2);
			//printf("left: %d %d\n", leftChannel, leftChannel_2);
			//printf("vol: %d, pan: %d\n", channelVolume, channelPan);
		}

	}
	return 0;
}