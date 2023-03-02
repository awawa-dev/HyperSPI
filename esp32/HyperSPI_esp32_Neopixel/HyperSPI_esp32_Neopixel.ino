#include <NeoPixelBus.h>
#include <ESP32DMASPISlave.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////          CONFIG SECTION STARTS               /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define THIS_IS_RGBW	   // RGBW SK6812, otherwise comment it
#define COLD_WHITE		   // for RGBW (THIS_IS_RGBW enabled) select COLD version, comment it if NEUTRAL
bool skipFirstLed = false; // if set the first led in the strip will be set to black (for level shifters using sacrifice LED)
int serialSpeed = 115200;  // serial port speed, only for monitoring as it's SPI transfer
#define MAX_BUFFER 4608	   // max internal cyclic buffer size
#define DATA_PIN 2		   // PIN: data output for LED strip

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////            CONFIG SECTION ENDS               /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef THIS_IS_RGBW
	float whiteLimit = 1.0f;
	#ifdef COLD_WHITE
		uint8_t rCorrection = 0xA0; // adjust red   -> white in 0-0xFF range
		uint8_t gCorrection = 0xA0; // adjust green -> white in 0-0xFF range
		uint8_t bCorrection = 0xA0; // adjust blue  -> white in 0-0xFF range
	#else
		uint8_t rCorrection = 0xB0; // adjust red   -> white in 0-0xFF range
		uint8_t gCorrection = 0xB0; // adjust green -> white in 0-0xFF range
		uint8_t bCorrection = 0x70; // adjust blue  -> white in 0-0xFF range
	#endif
#endif

ESP32DMASPI::Slave slave;

static const uint32_t REAL_BUFFER = 1536;
static const uint32_t BUFFER_SIZE = REAL_BUFFER + 8;

uint8_t *spi_slave_tx_buf;
uint8_t *spi_slave_rx_buf;
constexpr uint8_t CORE_TASK_SPI_SLAVE{0};
constexpr uint8_t CORE_TASK_PROCESS_BUFFER{0};
static TaskHandle_t task_handle_wait_spi = 0;
static TaskHandle_t task_handle_process_buffer = 0;

int pixelCount = 0; // This is dynamic, don't change it

#ifdef THIS_IS_RGBW
	#define LED_TYPE NeoGrbwFeature
#else
	#define LED_TYPE NeoGrbFeature
#endif

NeoPixelBus<LED_TYPE, NeoEsp32Rmt0Ws2812xMethod> *strip = NULL;

void Init(int count)
{
	if (strip != NULL)
		delete strip;

	pixelCount = count;
	strip = new NeoPixelBus<LED_TYPE, NeoEsp32Rmt0Ws2812xMethod>(pixelCount, DATA_PIN);
	strip->Begin();
}

enum class AwaProtocol
{
	HEADER_A,
	HEADER_w,
	HEADER_a,
	HEADER_HI,
	HEADER_LO,
	HEADER_CRC,
	VERSION2_GAIN,
	VERSION2_RED,
	VERSION2_GREEN,
	VERSION2_BLUE,
	RED,
	GREEN,
	BLUE,
	FLETCHER1,
	FLETCHER2
};

// static data buffer for the loop
uint16_t topBufferIndex = 0;
uint16_t currentBufferIndex = 0;
uint8_t buffer[MAX_BUFFER];
AwaProtocol state = AwaProtocol::HEADER_A;
bool version2 = false;
uint8_t incoming_gain = 0;
uint8_t incoming_red = 0;
uint8_t incoming_green = 0;
uint8_t incoming_blue = 0;
uint8_t CRC = 0;
uint16_t count = 0;
uint16_t currentPixel = 0;
uint16_t fletcher1 = 0;
uint16_t fletcher2 = 0;

#ifdef THIS_IS_RGBW
	RgbwColor inputColor;
	// calibration for cold RGBW
	uint8_t wChannel[256];
	uint8_t rChannel[256];
	uint8_t gChannel[256];
	uint8_t bChannel[256];
#else
	RgbColor inputColor;
#endif

// stats
unsigned long log_start = 0;
unsigned long stat_start = 0;
uint16_t stat_good = 0;
uint16_t stat_bad = 0;
uint16_t stat_frames = 0;
uint16_t stat_final_good = 0;
uint16_t stat_final_bad = 0;
uint16_t stat_final_frames = 0;
bool wantShow = false;

inline void ShowMe()
{
	if (wantShow == true && strip != NULL && strip->CanShow())
	{
		stat_good++;
		wantShow = false;
		strip->Show();
	}
}

void mainLoop()
{
	unsigned long curTime = millis();

	// stats
	if (curTime - stat_start > 995)
	{
		if (stat_frames > 0 && stat_frames >= stat_good)
		{
			stat_final_good = stat_good + ((wantShow) ? 1 : 0);
			stat_final_frames = stat_frames;
			stat_final_bad = stat_bad;
		}

		stat_start = curTime;
		stat_good = 0;
		stat_bad = 0;
		stat_frames = 0;

		if (curTime - log_start >= 2000)
		{
			Serial.write("HyperSPI version 6.\r\nStatistics for the last full 1 second cycle.\r\nFrames per second: ");
			Serial.print(stat_final_frames);
			Serial.write("\r\nGood frames: ");
			Serial.print(stat_final_good);
			Serial.write("\r\nBad frames:  ");
			Serial.print(stat_final_bad);
			Serial.write("\r\n-------------------------\r\n");
			log_start = curTime;
		}
	}

	if (state == AwaProtocol::HEADER_A)
		ShowMe();

	while (currentBufferIndex != topBufferIndex)
	{
		byte input = buffer[currentBufferIndex++];

		if (currentBufferIndex >= MAX_BUFFER)
			currentBufferIndex = 0;

		switch (state)
		{
		case AwaProtocol::HEADER_A:
			version2 = false;
			if (input == 'A')
				state = AwaProtocol::HEADER_w;
			break;

		case AwaProtocol::HEADER_w:
			if (input == 'w')
				state = AwaProtocol::HEADER_a;
			else
				state = AwaProtocol::HEADER_A;
			break;

		case AwaProtocol::HEADER_a:
			if (input == 'a')
				state = AwaProtocol::HEADER_HI;
			else if (input == 'A')
			{
				state = AwaProtocol::HEADER_HI;
				version2 = true;
			}
			else
				state = AwaProtocol::HEADER_A;
			break;

		case AwaProtocol::HEADER_HI:
			stat_frames++;
			currentPixel = 0;
			count = input * 0x100;
			CRC = input;
			fletcher1 = 0;
			fletcher2 = 0;
			state = AwaProtocol::HEADER_LO;
			break;

		case AwaProtocol::HEADER_LO:
			count += input;
			CRC = CRC ^ input ^ 0x55;
			state = AwaProtocol::HEADER_CRC;
			break;

		case AwaProtocol::HEADER_CRC:
			if (CRC == input)
			{
				if (count + 1 != pixelCount)
					Init(count + 1);

				state = AwaProtocol::RED;
			}
			else
				state = AwaProtocol::HEADER_A;
			break;

		case AwaProtocol::RED:
			inputColor.R = input;
			fletcher1 = (fletcher1 + (uint16_t)input) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;

			state = AwaProtocol::GREEN;
			break;

		case AwaProtocol::GREEN:
			inputColor.G = input;
			fletcher1 = (fletcher1 + (uint16_t)input) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;

			state = AwaProtocol::BLUE;
			break;

		case AwaProtocol::BLUE:
			inputColor.B = input;

			#ifdef THIS_IS_RGBW
				inputColor.W = min(rChannel[inputColor.R],
								min(gChannel[inputColor.G],
									bChannel[inputColor.B]));
				inputColor.R -= rChannel[inputColor.W];
				inputColor.G -= gChannel[inputColor.W];
				inputColor.B -= bChannel[inputColor.W];
				inputColor.W = wChannel[inputColor.W];
			#endif

			fletcher1 = (fletcher1 + (uint16_t)input) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;

			if (currentPixel == 0 && skipFirstLed)
			{
				#ifdef THIS_IS_RGBW
					strip->SetPixelColor(currentPixel++, RgbwColor(0, 0, 0, 0));
				#else
					strip->SetPixelColor(currentPixel++, RgbColor(0, 0, 0));
				#endif
			}
			else
				setStripPixel(currentPixel++, inputColor);

			if (count-- > 0)
				state = AwaProtocol::RED;
			else
			{
				if (version2)
					state = AwaProtocol::VERSION2_GAIN;
				else
					state = AwaProtocol::FLETCHER1;
			}

			break;

		case AwaProtocol::VERSION2_GAIN:
			incoming_gain = input;
			fletcher1 = (fletcher1 + (uint16_t)input) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;

			state = AwaProtocol::VERSION2_RED;
			break;

		case AwaProtocol::VERSION2_RED:
			incoming_red = input;
			fletcher1 = (fletcher1 + (uint16_t)input) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;

			state = AwaProtocol::VERSION2_GREEN;
			break;

		case AwaProtocol::VERSION2_GREEN:
			incoming_green = input;
			fletcher1 = (fletcher1 + (uint16_t)input) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;

			state = AwaProtocol::VERSION2_BLUE;
			break;

		case AwaProtocol::VERSION2_BLUE:
			incoming_blue = input;
			fletcher1 = (fletcher1 + (uint16_t)input) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;

			state = AwaProtocol::FLETCHER1;
			break;

		case AwaProtocol::FLETCHER1:
			if (input != fletcher1)
				state = AwaProtocol::HEADER_A;
			else
				state = AwaProtocol::FLETCHER2;
			break;

		case AwaProtocol::FLETCHER2:
			if (input == fletcher2)
			{
				wantShow = true;
				ShowMe();


				if (version2)
				{
					#ifdef THIS_IS_RGBW
						float final_limit = (incoming_gain != 255) ? incoming_gain / 255.0f : 1.0f;
						if (rCorrection != incoming_red || gCorrection != incoming_green || bCorrection != incoming_blue || whiteLimit != final_limit)
						{
							rCorrection = incoming_red;
							gCorrection = incoming_green;
							bCorrection = incoming_blue;
							whiteLimit = final_limit;
							prepareCalibration();
						}
					#endif
				}
			}

			state = AwaProtocol::HEADER_A;
			break;
		}
	}
}

#ifdef THIS_IS_RGBW
	inline void setStripPixel(uint16_t pix, RgbwColor &inputColor)
	{
		if (pix < pixelCount)
		{
			strip->SetPixelColor(pix, inputColor);
		}
	}
#else
	inline void setStripPixel(uint16_t pix, RgbColor &inputColor)
	{
		if (pix < pixelCount)
		{
			strip->SetPixelColor(pix, inputColor);
		}
	}
#endif

void setup()
{
	// Init serial port
	Serial.begin(serialSpeed);
	Serial.setTimeout(50);

	// Display config
	Serial.write("\r\nWelcome!\r\nAwa SPI driver 6.\r\n");

	// first LED info
	if (skipFirstLed)
		Serial.write("First LED: disabled\r\n");
	else
		Serial.write("First LED: enabled\r\n");

	// RGBW claibration info
	#ifdef THIS_IS_RGBW
		#ifdef COLD_WHITE
			Serial.write("Default color mode: RGBW cold\r\n");
		#else
			Serial.write("Default color mode: RGBW neutral\r\n");
		#endif
		prepareCalibration();
	#else
		Serial.write("Color mode: RGB\r\n");
	#endif

	// spi stuff
	spi_slave_tx_buf = slave.allocDMABuffer(BUFFER_SIZE);
	spi_slave_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);

	slave.setDataMode(SPI_MODE0);
	slave.setMaxTransferSize(BUFFER_SIZE);
	slave.setDMAChannel(1);
	slave.setQueueSize(1);
	slave.begin(VSPI);

	xTaskCreatePinnedToCore(task_wait_spi, "task_wait_spi", 2048, NULL, 2, &task_handle_wait_spi, CORE_TASK_SPI_SLAVE);
	xTaskNotifyGive(task_handle_wait_spi);
	xTaskCreatePinnedToCore(task_process_buffer, "task_process_buffer", 2048, NULL, 2, &task_handle_process_buffer, CORE_TASK_PROCESS_BUFFER);
}

void prepareCalibration()
{
	#ifdef THIS_IS_RGBW
		// prepare LUT calibration table, cold white is much better than "neutral" white
		for (uint32_t i = 0; i < 256; i++)
		{
			// color calibration
			float red = rCorrection * i;   // adjust red
			float green = gCorrection * i; // adjust green
			float blue = bCorrection * i;  // adjust blue

			wChannel[i] = (uint8_t)round(min(whiteLimit * i, 255.0f));
			rChannel[i] = (uint8_t)round(min(red / 0xFF, 255.0f));
			gChannel[i] = (uint8_t)round(min(green / 0xFF, 255.0f));
			bChannel[i] = (uint8_t)round(min(blue / 0xFF, 255.0f));
		}

		Serial.write("RGBW calibration. White limit(%): ");
		Serial.print(whiteLimit * 100.0f);
		Serial.write(" %, red: ");
		Serial.print(rCorrection);
		Serial.write(" , green: ");
		Serial.print(gCorrection);
		Serial.write(" , blue: ");
		Serial.print(bCorrection);
		Serial.write("\r\n");
	#endif
}

void readSpi()
{
	if (spi_slave_rx_buf[REAL_BUFFER] == 0xAA)
	{
		if (topBufferIndex + REAL_BUFFER < MAX_BUFFER)
		{
			memcpy(&(buffer[topBufferIndex]), spi_slave_rx_buf, REAL_BUFFER);
			topBufferIndex += REAL_BUFFER;
		}
		else
		{
			int left = MAX_BUFFER - topBufferIndex;
			memcpy(&(buffer[topBufferIndex]), spi_slave_rx_buf, left);
			memcpy(&(buffer[0]), spi_slave_rx_buf + left, REAL_BUFFER - left);
			topBufferIndex = REAL_BUFFER - left;
		}
		spi_slave_rx_buf[REAL_BUFFER] = 0;
	}
}

void task_wait_spi(void *pvParameters)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		slave.wait(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

		xTaskNotifyGive(task_handle_process_buffer);
	}
}

void task_process_buffer(void *pvParameters)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		readSpi();

		slave.pop();

		xTaskNotifyGive(task_handle_wait_spi);
	}
}

void loop()
{
	mainLoop();
}
