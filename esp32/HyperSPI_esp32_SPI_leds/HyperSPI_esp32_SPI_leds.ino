#include <NeoPixelBus.h>
#include <ESP32DMASPISlave.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////          CONFIG SECTION STARTS               /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool      skipFirstLed = false;  // if set the first led in the strip will be set to black (for level shifters)
int       serialSpeed = 115200;  // serial port speed, only for monitoring as it's SPI transfer
#define   MAX_BUFFER 4608        // max internal cyclic buffer size
#define   DATA_PIN   2           // PIN: data output for LED strip
#define   CLOCK_PIN  0           // PIN: clock output for LED strip

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////            CONFIG SECTION ENDS               /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
ESP32DMASPI::Slave slave;

static const uint32_t REAL_BUFFER = 1536;
static const uint32_t BUFFER_SIZE = REAL_BUFFER + 8;

uint8_t*              spi_slave_tx_buf;
uint8_t*              spi_slave_rx_buf;
constexpr uint8_t     CORE_TASK_SPI_SLAVE {0};
constexpr uint8_t     CORE_TASK_PROCESS_BUFFER {0};
static TaskHandle_t   task_handle_wait_spi = 0;
static TaskHandle_t   task_handle_process_buffer = 0;

int                   pixelCount  = 0;      // This is dynamic, don't change it

NeoPixelBus<DotStarBgrFeature, DotStarEsp32DmaHspiMethod>* strip = NULL;

void Init(int count)
{
    if (strip != NULL)
        delete strip;
        
    pixelCount = count;
    strip = new NeoPixelBus<DotStarBgrFeature, DotStarEsp32DmaHspiMethod>(pixelCount);
    strip->Begin(CLOCK_PIN, 12, DATA_PIN, 15);
}

enum class AwaProtocol {
    HEADER_A,
    HEADER_w,
    HEADER_a,
    HEADER_HI,
    HEADER_LO,
    HEADER_CRC,
    RED,
    GREEN,
    BLUE,
    FLETCHER1,
    FLETCHER2
};

// static data buffer for the loop
uint16_t    topBufferIndex = 0;
uint16_t    currentBufferIndex = 0;
uint8_t     buffer[MAX_BUFFER];
AwaProtocol state = AwaProtocol::HEADER_A;
uint8_t     CRC = 0;
uint16_t    count = 0;
uint16_t    currentPixel = 0;
uint16_t    fletcher1 = 0;
uint16_t    fletcher2 = 0;

RgbColor    inputColor;

// stats
unsigned long     log_start = 0;
unsigned long     stat_start  = 0;
uint16_t          stat_good = 0;
uint16_t          stat_bad = 0;
uint16_t          stat_frames  = 0;
uint16_t          stat_final_good = 0;
uint16_t          stat_final_bad = 0;
uint16_t          stat_final_frames  = 0;
bool              wantShow = false;

inline void ShowMe()
{
    if (wantShow == true && strip != NULL && strip->CanShow())
    {
        stat_good++;;
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
       if (stat_frames > 0 && stat_frames >= stat_good )
       {
           stat_final_good = stat_good + ((wantShow) ? 1 : 0);
           stat_final_frames = stat_frames;
           stat_final_bad = stat_bad;
       }
       
       stat_start  = curTime;
       stat_good   = 0;
       stat_bad    = 0;
       stat_frames = 0;

       if (curTime - log_start >= 2000)
       {
           Serial.write("HyperSPI version 1.\r\nStatistics for the last full 1 second cycle.\r\nFrames per second: ");       
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
                 if (input == 'A')
                     state = AwaProtocol::HEADER_w;
            break;

            case AwaProtocol::HEADER_w:
                 if (input == 'w')
                     state = AwaProtocol::HEADER_a;
                 else
                 {
                     state = AwaProtocol::HEADER_A;
                     stat_bad++;
                 }
            break;
            
            case AwaProtocol::HEADER_a:
                 if (input == 'a')
                     state = AwaProtocol::HEADER_HI;
                 else
                 {
                     state = AwaProtocol::HEADER_A;
                     stat_bad++;
                 }
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
                     if (count+1 != pixelCount)
                         Init(count+1);
                         
                     state = AwaProtocol::RED;
                 }
                 else
                 {
                     state = AwaProtocol::HEADER_A;
                     stat_bad++;
                 }
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
                        
                 fletcher1 = (fletcher1 + (uint16_t)input) % 255;
                 fletcher2 = (fletcher2 + fletcher1) % 255;

                 if (currentPixel == 0 && skipFirstLed)
                 {
                     strip->SetPixelColor(currentPixel++, RgbColor(0, 0, 0));
                 }
                 else
                     setStripPixel(currentPixel++, inputColor);

                 if (count-- > 0)
                     state = AwaProtocol::RED;
                 else
                     state = AwaProtocol::FLETCHER1;
            break;

            case AwaProtocol::FLETCHER1:
                 if (input != fletcher1)                 
                 {
                     state = AwaProtocol::HEADER_A;
                     stat_bad++;
                 }
                 else
                     state = AwaProtocol::FLETCHER2;
            break;

            case AwaProtocol::FLETCHER2:
                 if (input == fletcher2) 
                 {
                      wantShow = true;
                      ShowMe();            
                 }
                 else
                 {
                     stat_bad++;
                 }
                 state = AwaProtocol::HEADER_A;
            break;
        }
    }
}

inline void setStripPixel(uint16_t pix, RgbColor& inputColor)
{
    if (pix < pixelCount)
    {
        strip->SetPixelColor(pix, inputColor);
    }
}

void setup()
{
    // Init serial port
    Serial.begin(serialSpeed);
    Serial.setTimeout(50);  
  
    // Display config
    Serial.write("\r\nWelcome!\r\nAwa SPI driver.\r\n");

    Serial.write("Color mode: RGB SPI LED strip\r\n");    
    
    if (skipFirstLed)
        Serial.write("First LED: disabled\r\n");
    else
        Serial.write("First LED: enabled\r\n");

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

void task_wait_spi(void* pvParameters)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        slave.wait(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

        xTaskNotifyGive(task_handle_process_buffer);
    }
}

void task_process_buffer(void* pvParameters)
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
