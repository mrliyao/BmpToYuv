#include <fstream>
#include <iostream>

inline const std::string getImgName(const std::string& imgPath)
{
	auto iterTargetBeg = imgPath.rfind("\\");
	auto iterTargetEnd = imgPath.rfind(".bmp");
	return imgPath.substr(iterTargetBeg + 1, iterTargetEnd - iterTargetBeg - 1);
}

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		std::cout << "Please input the .bmp image path!" << std::endl;
		return -1;
	}
	// TODO: .bmp file format check
	// TODO: --dir args stetting
	std::string imgPath(argv[1]);
	std::ifstream ifstrm;
	ifstrm.open(imgPath, std::ios::in | std::ios::binary);
	if (!ifstrm)
	{
		// TODO: wrong image dir informing
		std::cout << "Can not open the image dir!" << std::endl;
		return -1;
	}
	// TODO: BM head byte check

	// get meta data
	constexpr uint16_t headerSize = 14; 
	constexpr uint16_t sizeByteAddrBeg = 2, sizeByteAddrEnd = 5;
	constexpr uint16_t imgDataAddrOffsetByteAddrBeg = 10, imgDataAddrOffsetByteAddrEnd = 13;

	char* bufferHeader = new char[256];
	ifstrm.read(bufferHeader, headerSize);
	
	uint32_t bmpSize = 0, imgDataAddrOffset = 0;
	memcpy(&bmpSize, bufferHeader + sizeByteAddrBeg, sizeByteAddrEnd - sizeByteAddrBeg);
	memcpy(&imgDataAddrOffset, bufferHeader + imgDataAddrOffsetByteAddrBeg, imgDataAddrOffsetByteAddrEnd - imgDataAddrOffsetByteAddrBeg);
	
	constexpr uint32_t widthByteAddrBeg = 18 - headerSize, widthByteAddrEnd = 21 - headerSize;
	constexpr uint32_t heightByteAddrBeg = 22 - headerSize, heightByteAddrEnd = 25 - headerSize;

	char* bufferImg = new char[bmpSize + 1024];
	ifstrm.read(bufferImg, bmpSize - headerSize);	

	uint32_t bmpWidth = 0, bmpHeight = 0;
	memcpy(&bmpWidth, bufferImg + widthByteAddrBeg, widthByteAddrEnd - widthByteAddrBeg);
	memcpy(&bmpHeight, bufferImg + heightByteAddrBeg, heightByteAddrEnd - heightByteAddrBeg);

	// TODO: check width % 4 == 0

	// get BGR data
	imgDataAddrOffset -= headerSize;
	char* bufferBGR = new char[bmpSize + 1024];
	memcpy(bufferBGR, bufferImg + imgDataAddrOffset, (bmpSize - headerSize) - imgDataAddrOffset);
	
	uint8_t* yBuf = new uint8_t[bmpHeight * bmpWidth];
	uint8_t* cbBuf = new uint8_t[bmpHeight * bmpWidth];
	uint8_t* crBuf = new uint8_t[bmpHeight * bmpWidth];

	for (uint16_t h = 0; h != bmpHeight; ++h)
	{
		for (uint16_t w = 0; w != bmpWidth; ++w)
		{
			// BGR, BGR, BGR, 3 bytes per pixel
			// in BMP, data arranges from buttom to top, from left to right
			uint32_t addrOffset = (bmpHeight - 1 - h) * bmpWidth * 3 + w * 3;
			uint8_t B = *(bufferBGR + addrOffset + 0);
			uint8_t G = *(bufferBGR + addrOffset + 1);
			uint8_t R = *(bufferBGR + addrOffset + 2);

			// RGB to YUV, as BT-709 Full Range Color Conversion ()
			/*float fY =   0.1829 * R + 0.6142 * G + 0.0620 * B + 16;
			  float fCb = -0.1007 * R - 0.3385 * G + 0.4392 * B + 128;
			  float fCr =  0.4392 * R - 0.3989 * G - 0.0403 * B + 128;*/

			// YUV444
			uint8_t& Y = *(yBuf + h * bmpWidth + w), & Cb = *(cbBuf + h * bmpWidth + w), & Cr = *(crBuf + h * bmpWidth + w);
			Y = (((47 * R + 157 * G + 16 * B + 128) >> 8) + 16);
			Cb = (((-26 * R - 87 * G + 112 * B + 128) >> 8) + 128);
			Cr = (((112 * R - 102 * G - 10 * B + 128) >> 8) + 128);
		}
	}

	// YUV plananr wirte out
	const std::string imgName = getImgName(imgPath);
	std::ofstream ofstrm(imgName + ".yuv", std::ios::app | std::ios::binary);
	if (ofstrm)
	{
		ofstrm.write(reinterpret_cast<const char*>(yBuf), bmpHeight * bmpWidth);
		ofstrm.write(reinterpret_cast<const char*>(cbBuf), bmpHeight * bmpWidth);
		ofstrm.write(reinterpret_cast<const char*>(crBuf),  bmpHeight * bmpWidth);
	}

	delete[] bufferHeader;
	delete[] bufferImg;
	delete[] yBuf;
	delete[] cbBuf;
	delete[] crBuf;
}