
#include <d3d9.h>
#include <xnamath.h>

#undef interface

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



union DXTColor
{
    struct {
        uint16_t red1    : 5;
        uint16_t green1  : 6;
        uint16_t blue1   : 5;
        uint16_t red2    : 5;
        uint16_t green2  : 6;
        uint16_t blue2   : 5;
    };
    struct {
        uint16_t color1;
        uint16_t color2;
    };
};

union BGRA
{
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    };
    uint8_t parts[4];
    uint32_t color;
};

struct DXT3Block
{
    uint64_t   alpha;
    DXTColor colors;
    uint32_t   indices;
};

struct BGR
{
    uint8_t   b;
    uint8_t   g;
    uint8_t   r;
};

struct RGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

int writeImage(const char* filename, int width, int height, BGR *buffer, uint8_t *alpha)
{
	int code = 0;
	FILE *fp = nullptr;
	png_structp png_ptr;
	png_infop info_ptr;
	// Open file for writing (binary mode)
	fopen_s(&fp, filename, "wb");

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);

	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, width, height,
		8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

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
	return 0;
}

void processDXTColor(BGR* p_colors, uint8_t* p_alphas, const DXTColor& p_blockColor, bool p_isDXT1)
{
    static XMFLOAT4 unpackedColorScale(255.f/31.f, 255.f/63.f, 255.f/31.f, 1.f);

    XMU565 color1Src(p_blockColor.color1);
    XMU565 color2Src(p_blockColor.color2);
    XMVECTOR colors[4];

    XMVECTOR colorScale = ::XMLoadFloat4(&unpackedColorScale);

    colors[0] = ::XMVectorSetW(::XMLoadU565(&color1Src), 255.0f);
    colors[0] = ::XMVectorSwizzle(colors[0], 2, 1, 0, 3);
    colors[0] = ::XMVectorMultiply(colors[0], colorScale);
    colors[1] = ::XMVectorSetW(::XMLoadU565(&color2Src), 255.0f);
    colors[1] = ::XMVectorSwizzle(colors[1], 2, 1, 0, 3);
    colors[1] = ::XMVectorMultiply(colors[1], colorScale);

    if (!p_isDXT1 || p_blockColor.color1 > p_blockColor.color2) {
        colors[2] = ::XMVectorLerp(colors[0], colors[1], (1.0f / 3.0f));
        colors[2] = ::XMVectorSetW(colors[2], 255.0f);
        colors[3] = ::XMVectorLerp(colors[0], colors[1], (2.0f / 3.0f));
        colors[3] = ::XMVectorSetW(colors[3], 255.0f);
    } else {
        colors[2] = ::XMVectorLerp(colors[0], colors[1], 0.5f);
        colors[2] = ::XMVectorSetW(colors[2], 255.0f);
        colors[3] = ::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    }

    for (uint32_t i = 0; i < 4; i++) {
        XMUBYTE4 color;
        ::XMStoreUByte4(&color, colors[i]);
        ::memcpy(&p_colors[i], &color, sizeof(p_colors[i]));
        if (p_alphas) { p_alphas[i] = color.w; }
    }
}

void processDXT5Block(BGR* p_colors, uint8_t* p_alphas, const DXT3Block& p_block, uint32_t p_blockX, uint32_t p_blockY, uint32_t p_width)
{
    uint32_t indices    = p_block.indices;
    uint64_t blockAlpha = p_block.alpha;
    BGR    colors[4];
    uint8_t  alphas[8];

    processDXTColor(colors, nullptr, p_block.colors, false);

    // Alpha 1 and 2
    alphas[0]    = (blockAlpha & 0xff);
    alphas[1]    = (blockAlpha & 0xff00) >> 8;
    blockAlpha >>= 16;
    // Alpha 3 to 8
    if (alphas[0] > alphas[1]) {
        for (uint32_t i = 2; i < 8; i++) {
            alphas[i] = ((8 - i) * alphas[0] + (i - 1) * alphas[1]) / 7;
        }
    } else {
        for (uint32_t i = 2; i < 6; i++) {
            alphas[i] = ((6 - i) * alphas[0] + (i - 1) * alphas[1]) / 5;
        }
        alphas[6] = 0x00;
        alphas[7] = 0xff;
    }

    for (uint32_t y = 0; y < 4; y++) {
        uint32_t curPixel = (p_blockY + y) * p_width + p_blockX;

        for (uint32_t x = 0; x < 4; x++) {
            BGR& color = p_colors[curPixel];
            uint8_t& alpha = p_alphas[curPixel];

            uint32_t index = (indices & 3);
            ::memcpy(&color, &colors[index], sizeof(color));
            alpha = alphas[blockAlpha & 7];

            curPixel++;
            indices    >>= 2;
            blockAlpha >>= 3;
        }
    }
}

void processDXT5(const BGRA* p_data, uint32_t p_width, uint32_t p_height, BGR *& po_colors, unsigned char*& po_alphas) {
    uint32_t numPixels    = (p_width * p_height);
    uint32_t numBlocks    = numPixels >> 4;
    const DXT3Block* blocks = reinterpret_cast<const DXT3Block*>(p_data);

    po_colors = new BGR[numPixels];
    po_alphas = new uint8_t[numPixels];

    const uint32_t numHorizBlocks = p_width >> 2;
    const uint32_t numVertBlocks  = p_height >> 2;

#pragma omp parallel for
    for (int y = 0; y < static_cast<int>(numVertBlocks); y++) {
        for (uint32_t x = 0; x < numHorizBlocks; x++)
        {
            const DXT3Block& block = blocks[(y * numHorizBlocks) + x];
            processDXT5Block(po_colors, po_alphas, block, x * 4, y * 4, p_width);
        }
    }
}

void readATEX(uint32_t width, uint32_t height, BGR*& po_colors, uint8_t*& po_alphas, void *d)
{
   // Assert(isValidHeader(m_data.GetPointer(), m_data.GetSize()));
   // auto atex = reinterpret_cast<const ANetAtexHeader*>(d);

    // Determine mipmap0 size and bail if the file is too small
    /*if (m_data.GetSize() >= sizeof(ANetAtexHeader) + sizeof(uint32)) {
        auto mipMap0Size = *reinterpret_cast<const uint32*>(&m_data[sizeof(ANetAtexHeader)]);

        if (mipMap0Size + sizeof(ANetAtexHeader) > m_data.GetSize()) {
            po_size.Set(0, 0);
            return false;
        }
    } else {
        po_size.Set(0, 0);
        return false;
    }*/

    // Create and init
    /*::SImageDescriptor descriptor;
    descriptor.xres = atex->width;
    descriptor.yres = atex->height;
    descriptor.Data = m_data.GetPointer();
    descriptor.imageformat = 0xf;
    descriptor.a = m_data.GetSize();
    descriptor.b = 6;
    descriptor.c = 0;*/

    // Init some fields
    auto data = reinterpret_cast<const BGRA *>(d);

    // Allocate output
    //auto output      = new BGRA[512*512];
    //descriptor.image = reinterpret_cast<unsigned char*>(output);

    // Uncompress
   /* switch (atex->formatInteger) {
    case FCC_DXT1:
        if (::AtexDecompress(data, m_data.GetSize(), 0xf, descriptor, reinterpret_cast<uint*>(output))) {
            this->processDXT1(output, atex->width, atex->height, po_colors, po_alphas);
        }
        break;
    case FCC_DXT2:
    case FCC_DXT3:
    case FCC_DXTN:
        if (::AtexDecompress(data, m_data.GetSize(), 0x11, descriptor, reinterpret_cast<uint*>(output))) {
            this->processDXT3(output, atex->width, atex->height, po_colors, po_alphas);
        }
        break;
    case FCC_DXT4:
    case FCC_DXT5:*/
       // if (::AtexDecompress(data, m_data.GetSize(), 0x13, descriptor, reinterpret_cast<uint*>(output))) {
            processDXT5(data, width, height, po_colors, po_alphas);
        //}
       // break;
    /*case FCC_DXTA:
        if (::AtexDecompress(data, m_data.GetSize(), 0x14, descriptor, reinterpret_cast<uint*>(output))) {
            this->processDXTA(reinterpret_cast<uint64*>(output), atex->width, atex->height, po_colors);
        }
        break;
    case FCC_DXTL:
        if (::AtexDecompress(data, m_data.GetSize(), 0x12, descriptor, reinterpret_cast<uint*>(output))) {
            this->processDXT5(output, atex->width, atex->height, po_colors, po_alphas);

            for (uint i = 0; i < (static_cast<uint>(atex->width) * static_cast<uint>(atex->height)); i++) {
                po_colors[i].r = (po_colors[i].r * po_alphas[i]) / 0xff;
                po_colors[i].g = (po_colors[i].g * po_alphas[i]) / 0xff;
                po_colors[i].b = (po_colors[i].b * po_alphas[i]) / 0xff;
            }
        }
        break;
    case FCC_3DCX:
        if (::AtexDecompress(data, m_data.GetSize(), 0x13, descriptor, reinterpret_cast<uint*>(output))) {
            this->process3DCX(reinterpret_cast<RGBA*>(output), atex->width, atex->height, po_colors, po_alphas);
        }
        break;
    default:
        freePointer(output);
        return false;
    }*/

    //delete output;

    /*if (po_colors) {
        po_size.Set(atex->width, atex->height);
        return true;
    }*/

}

int main(int argc, char** argv)
{
	const uint32_t aBufferSize = 1024 * 1024 * 30; // We make the assumption that no file is bigger than 30 M

    auto pANDatInterface = gw2dt::interface::createANDatInterface("T:\\Games\\Guild Wars 2\\Gw2.dat");

	const char *baseidstr = argv[1];
	int baseid = atoi(baseidstr);


    //auto aFileRecordVect = pANDatInterface->getFileRecordVect();

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

    // Open the given eula file
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
		CreateDirectoryA(oss2.str().c_str(), NULL);
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
				writeImage(filename.c_str(), mipmap.width(), mipmap.height(), colors, alphas);
				delete colors;
				delete alphas;
			} catch(std::exception& iException) {
				std::cout << "File " << it.fileId << " failed to decompress: " << std::string(iException.what()) << std::endl;
			}
		} else {
			//aOFStream.write(reinterpret_cast<const char*>(pOriBuffer), aOriSize);
		}
	}

    return 0;
}