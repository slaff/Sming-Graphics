/**
 * ILI9341.cpp
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Sming-Graphics Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 * @author: May 2021 - mikee47 <mike@sillyhouse.net>
 *
 * This code contains some reworked code from the Adafruit_ILI9341 display driver.
 * 
 * See https://github.com/adafruit/Adafruit_ILI9341
 *
 ****/

#include <Graphics/Display/ILI9341.h>
#include <Graphics/Renderer.h>
#include <Platform/System.h>
#include <Clock.h>

namespace Graphics
{
namespace Display
{
namespace
{
#define ILI9341_NOP 0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID 0x04
#define ILI9341_RDDST 0x09

#define ILI9341_SLPIN 0x10
#define ILI9341_SLPOUT 0x11
#define ILI9341_PTLON 0x12
#define ILI9341_NORON 0x13

#define ILI9341_RDMODE 0x0A
#define ILI9341_RDMADCTL 0x0B
#define ILI9341_RDPIXFMT 0x0C
#define ILI9341_RDIMGFMT 0x0D
#define ILI9341_RDSIGMODE 0x0E
#define ILI9341_RDSELFDIAG 0x0F

#define ILI9341_INVOFF 0x20
#define ILI9341_INVON 0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON 0x29

#define ILI9341_CASET 0x2A
#define ILI9341_PASET 0x2B
#define ILI9341_RAMWR 0x2C
#define ILI9341_COLSET 0x2D
#define ILI9341_RAMRD 0x2E
#define ILI9341_RAMRD_CONT 0x3E

#define ILI9341_PTLAR 0x30	///< Partial Area
#define ILI9341_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define ILI9341_MADCTL 0x36   ///< Memory Access Control
#define ILI9341_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define ILI9341_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR 0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1 0xC0
#define ILI9341_PWCTR2 0xC1
#define ILI9341_PWCTR3 0xC2
#define ILI9341_PWCTR4 0xC3
#define ILI9341_PWCTR5 0xC4
#define ILI9341_VMCTR1 0xC5
#define ILI9341_VMCTR2 0xC7

#define ILI9341_PWCTRA 0xCB
#define ILI9341_PWCTRB 0xCF

#define ILI9341_NVMEMWR 0xD0
#define ILI9341_NVMEMPK 0xD1
#define ILI9341_NVMEMST 0xD2

#define ILI9341_RDID4 0xD3
#define ILI9341_RDID1 0xDA
#define ILI9341_RDID2 0xDB
#define ILI9341_RDID3 0xDC

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define ILI9341_DRVTMA 0xE8
#define ILI9341_DRVTMB 0xEA
#define ILI9341_PWRSEQ 0xED

#define ILI9341_ENA3G 0xF2
#define ILI9341_IFCTL 0xF6
#define ILI9341_PMPRC 0xF7

// MADCTL register bits
#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04

const SpiDisplayList::Commands commands{
	.setColumn = ILI9341_CASET,
	.setRow = ILI9341_PASET,
	.readStart = ILI9341_RAMRD,
	.read = ILI9341_RAMRD_CONT,
	.writeStart = ILI9341_RAMWR,
};

// Command(1), length(2) data(length)
DEFINE_RB_ARRAY(													   //
	displayInitData,												   //
	DEFINE_RB_COMMAND(ILI9341_PWCTRA, 5, 0x39, 0x2C, 0x00, 0x34, 0x02) //
	DEFINE_RB_COMMAND(ILI9341_PWCTRB, 3, 0x00, 0XC1, 0X30)			   //
	DEFINE_RB_COMMAND(ILI9341_DRVTMA, 3, 0x85, 0x00, 0x78)			   //
	DEFINE_RB_COMMAND(ILI9341_DRVTMB, 2, 0x00, 0x00)				   //
	DEFINE_RB_COMMAND(ILI9341_PWRSEQ, 4, 0x64, 0x03, 0X12, 0X81)	   //
	DEFINE_RB_COMMAND(ILI9341_PMPRC, 1, 0x20)						   //
	DEFINE_RB_COMMAND(ILI9341_PWCTR1, 1, 0x23)						   // Power control: VRH[5:0]
	DEFINE_RB_COMMAND(ILI9341_PWCTR2, 1, 0x10)						   // Power control: SAP[2:0], BT[3:0]
	DEFINE_RB_COMMAND(ILI9341_VMCTR1, 2, 0x3e, 0x28)				   // VCM control: Contrast
	DEFINE_RB_COMMAND(ILI9341_VMCTR2, 1, 0x86)						   // VCM control2
	DEFINE_RB_COMMAND(ILI9341_MADCTL, 1, MADCTL_MX | MADCTL_BGR)	   // Orientation = normal
	DEFINE_RB_COMMAND(ILI9341_PIXFMT, 1, 0x55)						   // 16 bits per pixel
	DEFINE_RB_COMMAND(ILI9341_FRMCTR1, 2, 0x00, 0x18)				   //
	DEFINE_RB_COMMAND(ILI9341_DFUNCTR, 3, 0x08, 0x82, 0x27)			   // Display Function Control
	DEFINE_RB_COMMAND(ILI9341_ENA3G, 1, 0x00)						   // 3Gamma Function Disable
	DEFINE_RB_COMMAND(ILI9341_GAMMASET, 1, 0x01)					   // Gamma curve selected
	DEFINE_RB_COMMAND_LONG(ILI9341_GMCTRP1, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03,
						   0x0E, 0x09, 0x00) // Set Gamma
	DEFINE_RB_COMMAND_LONG(ILI9341_GMCTRN1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C,
						   0x31, 0x36, 0x0F) // Set Gamma
	DEFINE_RB_COMMAND(ILI9341_SLPOUT, 0)	 //
)

// Reading GRAM returns one byte per pixel for R/G/B (only top 6 bits are used, bottom 2 are clear)
static constexpr size_t READ_PIXEL_SIZE{3};

/**
 * @brief Manages completion of a `readDataBuffer` operation
 *
 * Performs format conversion, invokes callback (if provided) then releases shared buffer.
 *
 * @note Data is read back in RGB24 format, but written in RGB565.
 */
struct ReadPixelInfo {
	ReadBuffer buffer;
	size_t bytesToRead;
	ReadStatus* status;
	Surface::ReadCallback callback;
	void* param;

	static void IRAM_ATTR transferCallback(void* param)
	{
		System.queueCallback(taskCallback, param);
	}

	static void taskCallback(void* param)
	{
		auto info = static_cast<ReadPixelInfo*>(param);
		info->readComplete();
	}

	void readComplete()
	{
		if(buffer.format != PixelFormat::RGB24) {
			auto srcptr = &buffer.data[buffer.offset];
			auto dstptr = srcptr;
			if(buffer.format == PixelFormat::RGB565) {
				for(size_t i = 0; i < bytesToRead; i += READ_PIXEL_SIZE) {
					PixelBuffer buf;
					buf.rgb565.r = *srcptr++ >> 3;
					buf.rgb565.g = *srcptr++ >> 2;
					buf.rgb565.b = *srcptr++ >> 3;
					*dstptr++ = buf.u8[1];
					*dstptr++ = buf.u8[0];
				}
			} else {
				for(size_t i = 0; i < bytesToRead; i += READ_PIXEL_SIZE) {
					PixelBuffer buf;
					buf.rgb24.r = *srcptr++;
					buf.rgb24.g = *srcptr++;
					buf.rgb24.b = *srcptr++;
					dstptr += writeColor(dstptr, buf.color, buffer.format);
				}
			}
			bytesToRead = dstptr - &buffer.data[buffer.offset];
		}
		if(status != nullptr) {
			*status = ReadStatus{bytesToRead, buffer.format, true};
		}

		if(callback) {
			callback(buffer, bytesToRead, param);
		}

		buffer.data.release();
	}
};

} // namespace

class ILI9341Surface : public Surface
{
public:
	ILI9341Surface(ILI9341& device, size_t bufferSize)
		: device(device), displayList(commands, device.addrWindow, bufferSize)
	{
	}

	Type getType() const
	{
		return Type::Device;
	}

	Stat stat() const override
	{
		return Stat{
			.used = displayList.used(),
			.available = displayList.freeSpace(),
		};
	}

	void reset() override
	{
		displayList.reset();
	}

	Size getSize() const override
	{
		return device.getSize();
	}

	PixelFormat getPixelFormat() const override
	{
		return device.getPixelFormat();
	}

	bool setAddrWindow(const Rect& rect) override
	{
		return displayList.setAddrWindow(rect);
	}

	uint8_t* getBuffer(uint16_t minBytes, uint16_t& available) override
	{
		return displayList.getBuffer(minBytes, available);
	}

	void commit(uint16_t length) override
	{
		displayList.commit(length);
	}

	bool blockFill(const void* data, uint16_t length, uint32_t repeat) override
	{
		return displayList.blockFill(data, length, repeat);
	}

	bool writeDataBuffer(SharedBuffer& data, size_t offset, uint16_t length) override
	{
		return displayList.writeDataBuffer(data, offset, length);
	}

	bool setPixel(PackedColor color, Point pt) override
	{
		return displayList.setPixel(color, 2, pt);
	}

	/*
	 * The ILI9341 is fussy when reading GRAM.
	 *
	 *  - Pixels are read in RGB24 format, but written in RGB565.
	 * 	- The RAMRD command resets the read position to the start of the address window
	 *    so is used only for the first packet
	 *  - Second and subsequent packets use the RAMRD_CONT command
	 *  - Pixels must not be split across SPI packets so each packet can be for a maximum of 63 bytes (21 pixels)
	 */
	int readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param) override
	{
		// ILI9341 RAM read transactions must be in multiples of 3 bytes
		static constexpr size_t packetPixelBytes{63};

		auto pixelCount = (buffer.size() - buffer.offset) / READ_PIXEL_SIZE;
		if(pixelCount == 0) {
			debug_w("[readDataBuffer] pixelCount == 0");
			return 0;
		}
		auto& addrWindow = device.addrWindow;
		if(addrWindow.bounds.h == 0) {
			debug_w("[readDataBuffer] addrWindow.bounds.h == 0");
			return 0;
		}

		constexpr size_t hdrsize = DisplayList::codelen_readStart + DisplayList::codelen_read +
								   DisplayList::codelen_callback + sizeof(ReadPixelInfo);
		if(!displayList.require(hdrsize)) {
			debug_w("[readDataBuffer] no space");
			return -1;
		}
		if(!displayList.canLockBuffer()) {
			return -1;
		}
		if(buffer.format == PixelFormat::None) {
			buffer.format = PixelFormat::RGB24;
		}
		size_t maxPixels = (addrWindow.bounds.w * addrWindow.bounds.h) - addrWindow.column;
		pixelCount = std::min(maxPixels, pixelCount);
		ReadPixelInfo info{buffer, pixelCount * READ_PIXEL_SIZE, status, callback, param};
		if(status != nullptr) {
			*status = ReadStatus{};
		}

		auto bufptr = &buffer.data[buffer.offset];
		if(addrWindow.mode == AddressWindow::Mode::read) {
			displayList.readMem(bufptr, info.bytesToRead);
		} else {
			auto len = std::min(info.bytesToRead, packetPixelBytes);
			displayList.readMem(bufptr, len);
			if(len < info.bytesToRead) {
				displayList.readMem(bufptr + len, info.bytesToRead - len);
			}
		}
		addrWindow.seek(pixelCount);

		info.buffer.data.addRef();
		if(!displayList.writeCallback(info.transferCallback, &info, sizeof(info))) {
			debug_e("[DL] CALLBACK NO SPACE");
		}

		displayList.lockBuffer(buffer.data);
		return pixelCount;
	}

	bool render(const Object& object, const Rect& location, std::unique_ptr<Renderer>& renderer) override
	{
		// Small fills can be handled without using a renderer
		auto isSmall = [](const Rect& r) -> bool {
			constexpr size_t maxFillPixels{32};
			return (r.w * r.h) <= maxFillPixels;
		};

		switch(object.kind()) {
		case Object::Kind::FilledRect: {
			// Handle small transparent fills using display list
			auto obj = reinterpret_cast<const FilledRectObject&>(object);
			if(obj.radius != 0 || !obj.brush.isTransparent() || !isSmall(obj.rect)) {
				break;
			}
			auto color = obj.brush.getPackedColor(PixelFormat::RGB565);
			constexpr size_t bytesPerPixel{2};
			Rect absRect = obj.rect + location.topLeft();
			if(!absRect.clip(getSize())) {
				return true;
			}
			// debug_i("[ILI] HWBLEND (%s), %s", absRect.toString().c_str(), toString(color).c_str());
			return displayList.fill(absRect, color, bytesPerPixel, FillInfo::callbackRGB565);
		}
		default:;
		}

		return Surface::render(object, location, renderer);
	}

	bool present(PresentCallback callback, void* param) override
	{
		if(displayList.isBusy()) {
			debug_e("displayList BUSY, surface %p", this);
			return true;
		}
		if(displayList.isEmpty()) {
			// debug_d("displayList EMPTY, surface %p", this);
			return false;
		}
		device.execute(displayList, callback, param);
		return true;
	}

private:
	ILI9341& device;
	SpiDisplayList displayList;
};

bool ILI9341::begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t dcPin, uint8_t resetPin, uint32_t clockSpeed)
{
	if(!HSPI::Device::begin(pinSet, chipSelect)) {
		return false;
	}
	setSpeed(clockSpeed);
	setBitOrder(MSBFIRST);
	setClockMode(HSPI::ClockMode::mode0);
	setIoMode(HSPI::IoMode::SPI);

	this->dcPin = dcPin;
	pinMode(dcPin, OUTPUT);
	digitalWrite(dcPin, HIGH);
	dcState = true;
	onTransfer(transferBeginEnd);

	this->resetPin = resetPin;
	if(resetPin != PIN_NONE) {
		pinMode(resetPin, OUTPUT);
		reset(false);
		reset(true);
		delayMicroseconds(10000);
		reset(false);
		delayMicroseconds(1000);
	}

	SpiDisplayList list(commands, addrWindow, displayInitData);
	execute(list);

	// Final 'exit sleep' command takes a while
	delayMicroseconds(120000);

	HSPI::Request req;
	req.setCommand8(ILI9341_DISPON);
	execute(req);

	return true;
}

bool IRAM_ATTR ILI9341::transferBeginEnd(HSPI::Request& request)
{
	if(request.busy) {
		auto device = reinterpret_cast<ILI9341*>(request.device);
		auto newState = (request.cmdLen == 0);
		if(device->dcState != newState) {
			digitalWrite(device->dcPin, newState);
			device->dcState = newState;
		}
	}
	return true;
}

bool ILI9341::setOrientation(Orientation orientation)
{
	auto setMadCtl = [&](uint8_t value) -> bool {
		SpiDisplayList list(commands, addrWindow, 16);
		list.writeCommand(ILI9341_MADCTL, value, 1);
		execute(list);
		this->orientation = orientation;
		return true;
	};

	switch(orientation) {
	case Orientation::deg0:
		return setMadCtl(MADCTL_MX | MADCTL_BGR);
	case Orientation::deg90:
		return setMadCtl(MADCTL_MV | MADCTL_BGR);
	case Orientation::deg180:
		return setMadCtl(MADCTL_MY | MADCTL_BGR);
	case Orientation::deg270:
		return setMadCtl(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
	default:
		return false;
	}
}

Surface* ILI9341::createSurface(size_t bufferSize)
{
	return new ILI9341Surface(*this, bufferSize ?: 512U);
}

uint32_t ILI9341::readRegister(uint8_t cmd, uint8_t byteCount)
{
	HSPI::Request req;
	req.setCommand8(cmd);
	req.dummyLen = (byteCount > 2);
	req.in.set32(0, byteCount);
	execute(req);
	return req.in.data32;
}

uint32_t ILI9341::readDisplayId()
{
	return readRegister(ILI9341_RDDID, 4) >> 8;
}

uint32_t ILI9341::readDisplayStatus()
{
	return readRegister(ILI9341_RDDST, 4);
}

uint8_t ILI9341::readPowerMode()
{
	return readRegister(ILI9341_RDMODE, 1);
}

uint8_t ILI9341::readMADCTL()
{
	return readRegister(ILI9341_RDMADCTL, 1);
}

uint8_t ILI9341::readPixelFormat()
{
	return readRegister(ILI9341_RDPIXFMT, 1);
}

uint8_t ILI9341::readImageFormat()
{
	return readRegister(ILI9341_RDIMGFMT, 1);
}

uint8_t ILI9341::readSignalMode()
{
	return readRegister(ILI9341_RDSIGMODE, 1);
}

uint8_t ILI9341::readSelfDiag()
{
	return readRegister(ILI9341_RDSELFDIAG, 1);
}

uint16_t ILI9341::readNvMemStatus()
{
	return readRegister(ILI9341_NVMEMST, 3) >> 8;
}

} // namespace Display
} // namespace Graphics
