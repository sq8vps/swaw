#ifndef SCD40_H
#define SCD40_H

#include <stdint.h>
#include <stdbool.h>

void Scd40Init(void);

void Scd40HandleInterrupt(void);

void Scd40Process(void);

#endif //SCD40_H
