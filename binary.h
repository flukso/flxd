#ifndef BINARY_H
#define BINARY_H

extern const uint8_t bin2hex[];

void hexlify(uint8_t *bin, uint8_t *hex, size_t len);
bool unhexlify(uint8_t *hex, uint8_t *bin, size_t len);

#endif
