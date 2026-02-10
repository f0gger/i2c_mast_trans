#include <avr/io.h>
#include <stdint.h>

int main()
{
    DDRD &= ~(1 << DDD7);
    PORTD |= (1 << PORTD7);

    TWAR = 0x20;
    //TWCR &= ~((1 << TWSTA) | (1 << TWSTO));
    TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);

    uint8_t curr_status;
    while (1) {
        while ((TWSR & 0xf8) != 0xa8);
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
        //while (!(TWCR & (1 << TWINT)));

        curr_status = !(PIND & (1 << PIND7));
        TWDR = curr_status;
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
        while ((TWSR & 0xf8) != 0xb8);
    }
}