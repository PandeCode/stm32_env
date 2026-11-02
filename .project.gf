; gf2 reads this from the repo root, it asks once before trusting it.
; the Commands tab shows these as buttons
[commands]
Continue=c&
Step over=gf-next
Step in=gf-step
Step out=finish
Restart=monitor reset halt;tbreak main;c&
Reflash=monitor reset halt;load;monitor reset halt;tbreak main;c&
Stop on fail=b fail
Backtrace=bt
Registers=info registers
I2C1=svd I2C1
SPI1=svd SPI1
GPIOA=svd GPIOA
GPIOB=svd GPIOB
Clocks=svd RCC CR;svd RCC CCIPR3;svd RCC APB2ENR
Build all=shell just build
