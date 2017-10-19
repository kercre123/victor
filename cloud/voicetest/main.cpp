#include <stdio.h>
#include "audioUtil/audioCaptureSystem.h"

int main() {
	printf("Hello, world!\n");
	Anki::AudioUtil::AudioCaptureSystem captureSystem{1600};
	printf("I just created an audio capture system!\n");
	return 0;
}
