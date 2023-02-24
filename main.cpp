
#include "codec/aaccodec.h"
#include "codec/h264codec.h"
#include "encapsulation/mp4codec.h"

int main()
{
	aaccodec aac;
	aac.aacencodec("test/111.pcm", "test/111.aac");
//	aac.aacdecodec("test/111.aac", "test/111.pcm");
	h264codec h264;
//	h264.h264encodec("test.yuv", "test.h264");
//	h264.h264decedec("111.h264", "test/111.yuv");

//	cout << "HelloFFmpeg" << endl;
//	printf("%s", avcodec_configuration());

	return 0;
}