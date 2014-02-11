#include <cstdint>
#include <cstring>

#include "main.h"

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

struct Vec3 {
	Vec3() {
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
	}
	Vec3(float a, float b, float c) {
		data[0] = a;
		data[1] = b;
		data[2] = c;
	}
	void operator =(const Vec3 &other) {
		data[0] = other.data[0];
		data[1] = other.data[1];
		data[2] = other.data[2];
	}
	void swizzle(int a, int b, int c) {
		*this = Vec3(data[a], data[b], data[c]);
	}
	void multiply(Vec3 &other) {
		for (int i = 0; i < 3; ++i) {
			data[i] = data[i] * other.data[i];
		}
	}
	void lerp(Vec3 &other, float ammount) {
		for (int i = 0; i < 3; ++i) {
			data[i] = data[i] + ammount * (other.data[i] - data[i]);
		}
	}
	float data[3];
};


void processDXTColor(BGR* p_colors, uint8_t* p_alphas, const DXTColor& p_blockColor, bool p_isDXT1) {
	static Vec3 colorScale(255.f/31.f, 255.f/63.f, 255.f/31.f);

	Vec3 colors[4];
	colors[0] = Vec3(p_blockColor.red1, p_blockColor.green1, p_blockColor.blue1);
	colors[0].swizzle(2,1,0);
	colors[0].multiply(colorScale);

	colors[1] = Vec3(p_blockColor.red2, p_blockColor.green2, p_blockColor.blue2);
	colors[1].swizzle(2,1,0);
	colors[1].multiply(colorScale);

	if (!p_isDXT1 || p_blockColor.color1 > p_blockColor.color2) {
		colors[2] = colors[0];
		colors[2].lerp(colors[1], (1.0f / 3.0f));

		colors[3] = colors[0];
		colors[3].lerp(colors[1], (2.0f / 3.0f));
	} else {
        colors[2] = colors[0];
		colors[2].lerp(colors[1], (1.0f / 2.0f));
		colors[3] = Vec3();
    }

	for (uint32_t i = 0; i < 4; i++) {
		p_colors[i].b = (uint8_t)colors[i].data[0];
		p_colors[i].g = (uint8_t)colors[i].data[1];
		p_colors[i].r = (uint8_t)colors[i].data[2];
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
