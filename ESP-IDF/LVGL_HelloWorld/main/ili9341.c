/*
 * ili9341.c
 *
 *  Created on: Dec 16, 2023
 *      Author: xpress_embedo
 */
#include "ili9341.h"
#include "display_mng.h"
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Defines
#define TAG "ILI9341"

#define ILI9341_MADCTL_MY           (0x80u)   // Bottom to top
#define ILI9341_MADCTL_MX           (0x40u)   // Right to left
#define ILI9341_MADCTL_MV           (0x20u)   // Reverse Mode
#define ILI9341_MADCTL_ML           (0x10u)   // LCD refresh Bottom to top
#define ILI9341_MADCTL_RGB          (0x00u)   // Led-Green-Blue pixel order
#define ILI9341_MADCTL_BGR          (0x08u)   // Blue-Green-Red pixel order
#define ILI9341_MADCTL_MH           (0x04u)   // LCD refresh right to left

// structures
// The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct
typedef struct
{
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

// Private Function Prototypes
static void ili9341_reset(void);
static void ili9341_send_cmd(uint8_t cmd);
static void ili9341_send_data(void * data, uint16_t length);
static void ili9341_send_color(void * data, uint16_t length);

// Private Variables
static ili9341_orientation_e lcd_orientation = LCD_PORTRAIT;
static uint16_t lcd_width = ILI9341_LCD_WIDTH;
static uint16_t lcd_height = ILI9341_LCD_HEIGHT;

// Public Function Definition
void ili9341_init( void )
{
	lcd_init_cmd_t ili_init_cmds[]=
	{
    /* This is an un-documented command
    https://forums.adafruit.com/viewtopic.php?f=47&t=63229&p=320378&hilit=0xef+ili9341#p320378
    */
    {0xEF, {0x03, 0x80, 0x02}, 3},
	  /* Power contorl B, power control = 0, DC_ENA = 1 */
		{0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
		{0xED, {0x64, 0x03, 0X12, 0X81}, 4},
		/* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
		{0xE8, {0x85, 0x01, 0x79}, 3},
		/* Power control A, Vcore=1.6V, DDVDH=5.6V */
		{0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
		/* Pump ratio control, DDVDH=2xVCl */
		{0xF7, {0x20}, 1},
		/* Driver timing control, all=0 unit */
		{0xEA, {0x00, 0x00}, 2},
		/* Power control 1, GVDD=4.75V */
		{0xC0, {0x26}, 1},
		/* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
		{0xC1, {0x11}, 1},
		/* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
		{0xC5, {0x35, 0x3E}, 2},
		/* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
		{0xC7, {0xBE}, 1},
		/* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
		{0x36, {0x28}, 1},
		/* Pixel format, 16bits/pixel for RGB/MCU interface */
		{0x3A, {0x55}, 1},
		/* Frame rate control, f=fosc, 70Hz fps */
		{0xB1, {0x00, 0x1B}, 2},
		/* Enable 3G, disabled */
		{0xF2, {0x08}, 1},
		/* Gamma set, curve 1 */
		{0x26, {0x01}, 1},
		/* Positive gamma correction */
		{0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
		/* Negative gamma correction */
		{0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
		/* Column address set, SC=0, EC=0xEF */
		{0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
		/* Page address set, SP=0, EP=0x013F */
		{0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
		/* Memory write */
		{0x2C, {0}, 0},
		/* Entry mode set, Low vol detect disabled, normal display */
		{0xB7, {0x07}, 1},
		/* Display function control */
		{0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
		/* test command, need analysis */
		{0x36, {0x48}, 1},
		/* Sleep out */
		{0x11, {0}, 0x80},
		/* Display on */
		{0x29, {0}, 0x80},
		{0, {0}, 0xff},
	};

	//Initialize non-SPI GPIOs
	gpio_config_t io_conf = {};
	io_conf.pin_bit_mask = (1u<<ILI9341_DC);
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = true;
	gpio_config(&io_conf);
	// TODO: XE to be removed, alternative solution is written above
	// gpio_pad_select_gpio(ILI9341_DC);
	// gpio_set_direction(ILI9341_DC, GPIO_MODE_OUTPUT);

	ili9341_reset();

	vTaskDelay(500 / portTICK_PERIOD_MS);

	ESP_LOGI(TAG, "LCD ILI9341 Initialization.");

	// Send all the commands
	uint16_t cmd = 0;
	while (ili_init_cmds[cmd].databytes!=0xff)
	{
		ili9341_send_cmd( ili_init_cmds[cmd].cmd );
		ili9341_send_data( ili_init_cmds[cmd].data, (ili_init_cmds[cmd].databytes & 0x1F) );
		if (ili_init_cmds[cmd].databytes & 0x80)
		{
			vTaskDelay(200 / portTICK_PERIOD_MS);
		}
		cmd++;
	}
}

void ili9341_set_orientation( ili9341_orientation_e orientation )
{
  uint8_t data = 0x00;

  switch (orientation)
  {
  case LCD_ORIENTATION_0:
    data = (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
    lcd_width = ILI9341_LCD_WIDTH;
    lcd_height = ILI9341_LCD_HEIGHT;
    break;
  case LCD_ORIENTATION_90:
    data = (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
    lcd_width = ILI9341_LCD_HEIGHT;
    lcd_height = ILI9341_LCD_WIDTH;
    break;
  case LCD_ORIENTATION_180:
    data = (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
    lcd_width = ILI9341_LCD_WIDTH;
    lcd_height = ILI9341_LCD_HEIGHT;
    break;
  case LCD_ORIENTATION_270:
    data = (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
    lcd_width = ILI9341_LCD_HEIGHT;
    lcd_height = ILI9341_LCD_WIDTH;
    break;
  default:
    orientation = LCD_ORIENTATION_0;
    data = (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
    break;
  }
  lcd_orientation = orientation;
  ili9341_send_cmd(ILI9341_MAC);
  ili9341_send_data(&data, 1);
}

ili9341_orientation_e ili9341_get_orientation( void )
{
  return lcd_orientation;
}

// Set the display area
void ili9341_set_window( uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end )
{
  uint8_t params[4] = { 0 };
  // column address set
  params[0] = x_start >> 8u;
  params[1] = 0xFF & x_start;
  params[2] = x_end >> 8u;
  params[3] = 0xFF & x_end;
  ili9341_send_cmd(ILI9341_CASET);
  ili9341_send_data( params, 4u );

  // Row Address Set (2B) also called as page address set
  params[0] = y_start >> 8u;
  params[1] = 0xFF & y_start;
  params[2] = y_end >> 8u;
  params[3] = 0xFF & y_end;
  ili9341_send_cmd( ILI9341_RASET );
  ili9341_send_data( params, 4u );
}

void ili9341_draw_pixel( uint16_t x, uint16_t y, uint16_t color )
{
  uint8_t data[2] = { (color>>8u), (color & 0xFF) };
  ili9341_set_window( x, y, x, y);
  // ILI9341_SendCommand(ILI9341_GRAM, 0u, 0u );
  // ILI9341_SendData( data, 2u);
  // OR
  ili9341_send_cmd(ILI9341_GRAM);
  ili9341_send_data(data, 2u );
}

//void ili9341_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
//{
//	uint8_t data[4];
//
//	// Column addresses
//	ili9341_send_cmd(0x2A);
//	data[0] = (area->x1 >> 8) & 0xFF;
//	data[1] = area->x1 & 0xFF;
//	data[2] = (area->x2 >> 8) & 0xFF;
//	data[3] = area->x2 & 0xFF;
//	ili9341_send_data(data, 4);
//
//	// Page addresses
//	ili9341_send_cmd(0x2B);
//	data[0] = (area->y1 >> 8) & 0xFF;
//	data[1] = area->y1 & 0xFF;
//	data[2] = (area->y2 >> 8) & 0xFF;
//	data[3] = area->y2 & 0xFF;
//	ili9341_send_data(data, 4);
//
//	// Memory write
//	ili9341_send_cmd(0x2C);
//	uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);
//	ili9341_send_color((void*)color_map, size * 2);
//}


// Private Function Definitions
static void ili9341_reset(void)
{
  // ili9341 software reset command
  ili9341_send_cmd(0x01);
}

static void ili9341_send_cmd(uint8_t cmd)
{
  display_send_cmd( cmd );
}

static void ili9341_send_data(void * data, uint16_t length)
{
  display_send_data(data, length);
}

static void ili9341_send_color(void * data, uint16_t length)
{
}

