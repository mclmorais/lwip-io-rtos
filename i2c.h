#if !defined(__I2C_H__)
#define __I2C_H__
#include "utils.h"
#include "./images/images.h"
#include "./fonts/fonts.h"
#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "./ssd1306.h"

#include <string.h>


void I2C_Init(void);
void I2C_OLED_Send(uint8_t type, uint8_t command);
void I2C_OLED_Init(void);
void I2C_OLED_Draw(const uint8_t*, uint32_t);
void I2C_OLED_Print(char *string);
void I2C_OLED_Move_Cursor(uint8_t row, uint8_t column);
void I2C_OLED_Set_Contrast(uint8_t contrast_level);
void I2C_OLED_Clear(void);
void I2C_OLED_Sequence_Init(void);
void IC_Tester_Select(void);
uint8_t center_string_position(char* string);

extern uint8_t update_digit;

#endif // __I2C_H__
