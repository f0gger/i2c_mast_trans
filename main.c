#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define TIMEOUT 250

void noResponse();

int main()
{
    DDRD = 0;
    PORTD = 0xff;

    DDRB = 0;
    PORTB = 0xff;

    DDRC |= (1 << PC1);
    PORTC |= (1 << PC1); //powerup and status led

    TWAR = 0x20<<1;

    TWBR = 0;

    TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);

    uint8_t d_snap;
    uint8_t b_snap;

    volatile uint16_t i = 0;
    volatile uint8_t cnt = 0;

    while (1) {
        i = 0;
        cnt = 0;
        while (!(TWCR & (1 << TWINT)))
        {
            if (i == UINT16_MAX)
            {
                i = 0;
                cnt++;
            }
            if (cnt > TIMEOUT)
            {
                cnt = 0;
                noResponse();
            }
            i++;
        }     

        uint8_t status = TWSR & 0xf8;

        switch (status)
        {
            case 0x60:
                TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
                break;

            case 0xa8:   
            case 0xb0: 
                d_snap = ~PIND;
                b_snap = ~PINB;
                TWDR = d_snap;
                TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
                break;

            case 0xb8:
                TWDR = b_snap;
                TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);              
                break;

            case 0xc0:
            case 0xc8:
                TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
                break;

            default:
                TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
                break;

        }
    }
}

void noResponse()
{
    while (1)
    {
        PORTC ^= (1 << PC1);
        _delay_ms(700);
    }
}
