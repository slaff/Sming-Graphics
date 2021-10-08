#include <SmingCore.h>
#include <Graphics.h>
#include <Graphics/Debug.h>
#include <Graphics/Drawing/Macros.h>
#include <Graphics/SampleConfig.h>

/*
 This commend is taken from the wonderful TFT_eSPI project.
 See original at:
 https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Test%20and%20diagnostics/Colour_Test/Colour_Test.ino

 Different hardware manufacturers use different colour order
 configurations at the hardware level.  This may result in
 incorrect colours being displayed.
 Incorrectly displayed colours could also be the result of
 using the wrong display driver in the library setup file.
 Typically displays have a control register (MADCTL) that can
 be used to set the Red Green Blue (RGB) colour order to RGB
 or BRG so that red and blue are swapped on the display.
 This control register is also used to manage the display
 rotation and coordinate mirroring. The control register
 typically has 8 bits, for the ILI9341 these are:
 Bit Function
 7   Mirror Y coordinate (row address order)
 6   Mirror X coordinate (column address order)
 5   Row/column exchange (for rotation)
 4   Refresh direction (top to bottom or bottom to top in portrait orientation)
 3   RGB order (swaps red and blue)
 2   Refresh direction (top to bottom or bottom to top in landscape orientation)
 1   Not used
 0   Not used
 The control register bits can be written with this example command sequence:

    tft.writecommand(TFT_MADCTL);
    tft.writedata(0x48);          // Bits 6 and 3 set

 0x48 is the default value for ILI9341 (0xA8 for ESP32 M5STACK)
 in rotation 0 orientation.

 Another control register can be used to "invert" colours,
 this swaps black and white as well as other colours (e.g.
 green to magenta, red to cyan, blue to yellow).

 */

using Color = Graphics::Color;
using FontStyle = Graphics::FontStyle;

using namespace Graphics;

namespace
{

RenderQueue renderQueue(tft);

void done(SceneObject* scene)
{
	Serial.println("Scene ready");
}

void render()
{
	tft.setOrientation(Orientation::deg270);
	auto size = tft.getSize();
	auto scene = new SceneObject(size, F("Color Tests"));
	scene->clear();

	Color color[] = {Color::RED,  Color::GREEN, Color::BLUE, Color::WHITE};
	uint8_t numColors = sizeof(color) / sizeof(Color);

	auto height = size.h;
	auto width = size.w / numColors;

	for(unsigned i=0; i< numColors; i++) {
		scene->fillRect(i * width, 0, width, height, color[i]);
	}

	renderQueue.render(scene, done);
}

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Allow debug output to serial

#ifndef DISABLE_WIFI
	//WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(false);
	WifiAccessPoint.enable(false);
#endif

#ifdef ARCH_HOST
	setDigitalHooks(nullptr);
#endif

	Serial.println("Display start");
	initDisplay();

	render();
}
