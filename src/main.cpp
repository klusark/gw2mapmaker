#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#include <iostream>

#include <gw2formats/pf/PagedImagePackFile.h>
#include <gw2formats/TextureFile.h>

#include <iostream>
#include <cstdint>

#include <sstream>
#include <fstream>
#include <png.h>

#include "gw2DatTools/interface/ANDatInterface.h"
#include "gw2DatTools/compression/inflateDatFileBuffer.h"
#include "gw2DatTools/compression/inflateTextureFileBuffer.h"

#include "main.h"
#include "dxt.h"


int writeImage(const char* filename, int width, int height, BGR *buffer, uint8_t *alpha, char *pimg)
{
	int code = 0;
	FILE *fp = nullptr;
	png_structp png_ptr;
	png_infop info_ptr;
	// Open file for writing (binary mode)
	fp = fopen(filename, "wb");

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);

	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, width, height,
		8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_text text;
	text.compression = PNG_TEXT_COMPRESSION_NONE;
	char buff[16];
	strcpy(buff, "Comment");
	text.key = buff;
	text.text = pimg;
	text.text_length = strlen(text.text);

	png_set_text(png_ptr, info_ptr, (png_const_textp)&text, 1);

	png_write_info(png_ptr, info_ptr);

	RGBA *data = new RGBA[width*height];

	for (int i = 0; i < width*height; ++i) {
		data[i].a = alpha[i];
		data[i].r = buffer[i].b;
		data[i].g = buffer[i].g;
		data[i].b = buffer[i].r;
	}


	// Write image data

	for (int y=0 ; y<height ; y++) {
		png_write_row(png_ptr, (uint8_t *)data + y*width*4);
	}
	png_write_end(png_ptr, NULL);
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	delete[] data;
	return 0;
}

int main(int argc, char** argv)
{
	const uint32_t aBufferSize = 1024 * 1024 * 30; // We make the assumption that no file is bigger than 30 M

	auto pANDatInterface = gw2dt::interface::createANDatInterface("/home/joel/Gw2.dat");

	char *baseidstr = argv[1];
	int baseid = atoi(baseidstr);

	uint8_t* pOriBuffer = new uint8_t[aBufferSize];
	uint8_t* pInfBuffer2 = new uint8_t[aBufferSize];
	uint8_t* pInfBuffer = new uint8_t[aBufferSize];
	uint8_t* pimgbuff = new uint8_t[64*1024];
	uint8_t* pimgbuff2 = new uint8_t[64*1024];

	uint32_t pimgbuffsize = 64*1024;
	uint32_t pimgbuffsize2 = 64*1024;

	auto pimg = pANDatInterface->getFileRecordForBaseId(baseid);
	pANDatInterface->getBuffer(pimg, pimgbuffsize, pimgbuff);
	gw2dt::compression::inflateDatFileBuffer(pimgbuffsize, pimgbuff, pimgbuffsize2, pimgbuff2);

	gw2f::pf::PimgPackFile eula(pimgbuff2, pimgbuffsize2);
	auto chunk = eula.chunk<gw2f::pf::PagedImageChunks::PGTB>();

	auto pages = chunk.get()->strippedPages;
	for (uint32_t i = 0; i < pages.size(); ++i) {
		auto page = pages[i];
		auto coord = page.coord;
		int c = page.filename.lowPart();
		int a = page.filename.fileId();
		if (a == 0) {
			continue;
		}


		uint32_t aOriSize = aBufferSize;
		auto it = pANDatInterface->getFileRecordForBaseId(a);
		pANDatInterface->getBuffer(it, aOriSize, pOriBuffer);

		std::cout << "Processing File " << a << std::endl;

		//std::ofstream aOFStream;
		std::ostringstream oss2;
		oss2 << coord[1];
		const char *dirname = oss2.str().c_str();
#ifdef WIN32
		CreateDirectoryA(dirname, NULL);
#else
		mkdir(dirname, 0755);
#endif
		//system(oss2.str().c_str());
		//CreateDirectory(oss2.str(), NULL);
		std::ostringstream oss;

		oss << coord[1] << "/" << coord[0] << ".png";
		std::string filename = oss.str();
		//aOFStream.open(oss.str().c_str(), std::ios::binary | std::ios::out);

		if (aOriSize == aBufferSize) {
			std::cout << "File " << it.fileId << " has a size greater than (or equal to) 30Mo." << std::endl;
		}

		if (it.isCompressed) {
			uint32_t aInfSize = aBufferSize;
			uint32_t aInfSize2 = aBufferSize;

			BGR* colors   = nullptr;
			uint8_t* alphas = nullptr;

			try	{
				gw2dt::compression::inflateDatFileBuffer(aOriSize, pOriBuffer, aInfSize, pInfBuffer);
				gw2f::TextureFile tex(pInfBuffer, aInfSize);
				auto &mipmap = tex.mipMapLevel(0);
				gw2dt::compression::inflateTextureBlockBuffer(mipmap.width(), mipmap.height(), mipmap.format(), mipmap.size(), mipmap.data(), aInfSize2, pInfBuffer2);
				readATEX(mipmap.width(), mipmap.height(), colors, alphas, pInfBuffer2);
				//gw2dt::compression::inflateTextureFileBuffer(aInfSize, pInfBuffer, aInfSize2, pInfBuffer2);
				//aOFStream.write(reinterpret_cast<const char*>(colors), 512*512*3);
				writeImage(filename.c_str(), mipmap.width(), mipmap.height(), colors, alphas, baseidstr);
				delete[] colors;
				delete[] alphas;
			} catch(std::exception& iException) {
				std::cout << "File " << it.fileId << " failed to decompress: " << std::string(iException.what()) << std::endl;
			}
		} else {
			//aOFStream.write(reinterpret_cast<const char*>(pOriBuffer), aOriSize);
		}
	}
	delete[] pOriBuffer;
	delete[] pInfBuffer2;
	delete[] pInfBuffer;
	delete[] pimgbuff;
	delete[] pimgbuff2;
    
	return 0;
}
