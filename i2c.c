#include "./i2c.h"

#define LCD_OFFLINE

#ifdef LCD_OFFLINE
    char offline_strings[50][16];
    uint8_t offline_iterator = 0;
#endif


uint32_t error = 0;
uint32_t busy = 0;

extern uint8_t update_digit;

void I2C_Init(void)
{
    //Ativa clock do módulo I2C 0
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;

    //Espera até módulo 0 estar pronto para uso
    while ((SYSCTL_RCGCI2C_R & SYSCTL_RCGCI2C_R0) != SYSCTL_RCGCI2C_R0)
    {
    };

    //Ativa função de master no módulo 0
    I2C0_MCR_R |= I2C_MCR_MFE;

    //2*(TPR+1)*10*12,5ns = 2,5us -> TPR = 9 (400kbps)
    //I2C0_MTPR_R &= ~I2C_MTPR_TPR_M;
    I2C0_MTPR_R = 9;
}

void I2C_Check_Transmission(void)
{
    //Checks Master Control/Status register BUSY and ERROR bits
    while (I2C0_MCS_R & I2C_MCS_BUSY)
    {
    }

    if (I2C0_MCS_R & I2C_MCS_ERROR)
    {
        if (!I2C0_MCS_R & I2C_MCS_ARBLST)
        {
            I2C0_MCS_R = (I2C_MCS_STOP);
        }
        error++;
    }
}

void I2C_OLED_Send(uint8_t type, uint8_t command_byte)
{

    //--------Transmission of CONTROL byte--------
    //Puts address and transmit bit on the Master Slave Address Register
    I2C0_MSA_R = (SSD1306_I2C_ADDRESS << 1) | 0x00;

    //Writes control byte
    I2C0_MDR_R = type;

    while (I2C0_MCS_R & I2C_MCS_BUSBSY)
        busy++;

    //Writes ---01011 to Master Control/Status register
    I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_START | I2C_MCS_RUN);

    I2C_Check_Transmission();

    SysTick_Wait1us(SSD1306_COMMAND_DELAY);

    //--------Transmission of COMMAND byte--------

    //Writes command byte
    I2C0_MDR_R = command_byte;

    //Writes  ---01101 to Master Control/Status register
    I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_STOP | I2C_MCS_RUN);

    I2C_Check_Transmission();

    SysTick_Wait1us(SSD1306_COMMAND_DELAY);
}

void I2C_OLED_Init(void)
{
    I2C_OLED_Send(SSD1306_COMMAND, 0xAE); // Set display OFF

    I2C_OLED_Send(SSD1306_COMMAND, 0xD5); // Set Display Clock Divide Ratio / OSC Frequency
    I2C_OLED_Send(SSD1306_COMMAND, 0x80); // Display Clock Divide Ratio / OSC Frequency

    I2C_OLED_Send(SSD1306_COMMAND, 0xA8); // Set Multiplex Ratio
    I2C_OLED_Send(SSD1306_COMMAND, 0x3F); // Multiplex Ratio for 128x64 (64-1)

    I2C_OLED_Send(SSD1306_COMMAND, 0xD3); // Set Display Offset
    I2C_OLED_Send(SSD1306_COMMAND, 0x00); // Display Offset

    I2C_OLED_Send(SSD1306_COMMAND, 0x40); // Set Display Start Line

    I2C_OLED_Send(SSD1306_COMMAND, 0x8D); // Set Charge Pump
    I2C_OLED_Send(SSD1306_COMMAND, 0x14); // Charge Pump (0x10 External, 0x14 Internal DC/DC)

    I2C_OLED_Send(SSD1306_COMMAND, SSD1306_MEMORYMODE); // 0x20
    I2C_OLED_Send(SSD1306_COMMAND, 0x00);

    I2C_OLED_Send(SSD1306_COMMAND, 0xA1); // Set Segment Re-Map
    I2C_OLED_Send(SSD1306_COMMAND, 0xC8); // Set Com Output Scan Direction

    I2C_OLED_Send(SSD1306_COMMAND, 0xDA); // Set COM Hardware Configuration
    I2C_OLED_Send(SSD1306_COMMAND, 0x12); // COM Hardware Configuration

    I2C_OLED_Send(SSD1306_COMMAND, 0x81); // Set Contrast
    I2C_OLED_Send(SSD1306_COMMAND, 0xCF); // Contrast

    I2C_OLED_Send(SSD1306_COMMAND, 0xD9); // Set Pre-Charge Period
    I2C_OLED_Send(SSD1306_COMMAND, 0xF1); // Set Pre-Charge Period (0x22 External, 0xF1 Internal)

    I2C_OLED_Send(SSD1306_COMMAND, 0xDB); // Set VCOMH Deselect Level
    I2C_OLED_Send(SSD1306_COMMAND, 0x40); // VCOMH Deselect Level

    I2C_OLED_Send(SSD1306_COMMAND, 0xA4); // Set display fetch from RAM
    I2C_OLED_Send(SSD1306_COMMAND, 0xA6); // Set display not inverted
    I2C_OLED_Send(SSD1306_COMMAND, 0xAF); // Set display On
}

void I2C_OLED_Move_Cursor(uint8_t row, uint8_t column)
{
    I2C_OLED_Send(SSD1306_COMMAND, SSD1306_PAGEADDR);
    I2C_OLED_Send(SSD1306_COMMAND, row);
    I2C_OLED_Send(SSD1306_COMMAND, 7);

    I2C_OLED_Send(SSD1306_COMMAND, SSD1306_COLUMNADDR);
    I2C_OLED_Send(SSD1306_COMMAND, column);
    I2C_OLED_Send(SSD1306_COMMAND, SSD1306_LCDWIDTH - 1);
}

void I2C_OLED_Print(char* string)
{

    #ifdef LCD_OFFLINE
        strcpy(offline_strings[offline_iterator], string);
        strcpy(offline_strings[offline_iterator+1], " ");
        if(++offline_iterator >= 50) offline_iterator = 0;
    #endif


    while (*string != '\0')
    {
        I2C_OLED_Draw(font_default+(*string*8),8);
        string++;
    }
}

void I2C_OLED_Draw(const uint8_t *data_pointer, uint32_t data_size)
{
    I2C0_MSA_R = (SSD1306_I2C_ADDRESS << 1) | 0x00;

    I2C0_MDR_R = SSD1306_DATA;

    while (I2C0_MCS_R & I2C_MCS_BUSBSY)
        busy++;

    I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_START | I2C_MCS_RUN);

    I2C_Check_Transmission();

    SysTick_Wait1us(SSD1306_COMMAND_DELAY);

    int i = 0;
    for (i = 0; i < data_size - 1; i++)
    {
        I2C0_MDR_R = data_pointer[i];

        I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_RUN);

        SysTick_Wait1us(SSD1306_DATA_DELAY);
        I2C_Check_Transmission();
    }

    I2C0_MDR_R = data_pointer[data_size - 1];

    I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_STOP | I2C_MCS_RUN);

    I2C_Check_Transmission();
}

void I2C_OLED_Set_Contrast(uint8_t contrast_level)
{
    I2C_OLED_Send(SSD1306_COMMAND, SSD1306_SETCONTRAST);
    I2C_OLED_Send(SSD1306_COMMAND, contrast_level);
}

void I2C_OLED_Clear()
{
    I2C_OLED_Move_Cursor(0,0);

    I2C0_MSA_R = (SSD1306_I2C_ADDRESS << 1) | 0x00;

    I2C0_MDR_R = SSD1306_DATA;

    while (I2C0_MCS_R & I2C_MCS_BUSBSY)
        busy++;

    I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_START | I2C_MCS_RUN);

    I2C_Check_Transmission();

    SysTick_Wait1us(SSD1306_COMMAND_DELAY);
    int i = 0;
    for (i = 0; i < 1023; i++)
    {
        I2C0_MDR_R = 0x00;

        I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_RUN);

        SysTick_Wait1us(SSD1306_DATA_DELAY);

        I2C_Check_Transmission();
    }

    I2C0_MDR_R = 0x00;

    I2C0_MCS_R = (I2C_MCS_ACK | I2C_MCS_STOP | I2C_MCS_RUN);

    I2C_Check_Transmission();
}

void I2C_OLED_Sequence_Init(void)
{
    I2C_OLED_Move_Cursor(0,0);
    I2C_OLED_Draw(((uint8_t *)BMP_UTFPR), 1024);
   SysTick_Wait1ms(2000);
   I2C_OLED_Clear();
}

uint8_t center_string_position(char* string)
{
    //WIDTH - 8 * (length / 2)
    return 64 - strlen(string) * 4;
}
