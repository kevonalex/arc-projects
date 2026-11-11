# Pack 08/02 cheatsheet — interrupts, peripherals, DMA (every line cites the worksheet section)

Host-simulated. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| NVIC priority: only the top N bits implemented; level = raw >> (8−N) | §3.1, Q1 |
| Lower number = more urgent; equal priority never preempts (tail-chains) | §3.1, Q1, Q2 |
| Pending order: lowest raw priority, then lowest IRQ number | §3.1, Q3 |
| FreeRTOS task priority is the opposite way round (higher = more urgent) | §3.1 |
| ISR: short, clear flag, no blocking/malloc/printf, atomics/ring, defer work | §3.1, Q4 |
| UART 8N1 = 10 bit times; 115200 → 86.8 µs/byte; 9600 → 1041.7 µs; 11-bit → 95.5 µs | §3.2, Q6 |
| BRR = round(fclk / baud); actual = fclk / BRR; tolerance ≈ ±2–3 % end-to-end | §3.2, Q5 |
| 72 M/115200 → 625 exact; 16 M → 139 (−0.08 %); 8 M → 69 (+0.644 %) | §3.2, Q5 |
| SPI: full duplex shift ring; 8 clocks swap bytes; every read is a write | §3.3, Q7 |
| SPI mode = CPOL·2 + CPHA; sample on rising iff CPOL == CPHA | §3.3, Q8 |
| SPI time = bytes·8 / SCK (4 B @ 1 MHz = 32 µs) | §3.3 |
| I2C first byte = addr7<<1 \| R/W (0x50 → 0xA0/0xA1; 0x68 → 0xD0) | §3.3, Q9 |
| I2C 9 bit times per byte (ACK); 2 data bytes @100 kHz = 270 µs | §3.3, Q9 |
| f_timer = fclk / ((PSC+1)(ARR+1)); both 16-bit | §3.4, Q10 |
| 72 MHz: 8 kHz → PSC 0, ARR 8999; 1 kHz → PSC 1, ARR 35999; 1 Hz → PSC 1124, ARR 63999 | §3.4, Q10 |
| ADC LSB = Vref/2^N; full-scale code = Vref − 1 LSB (4095 → 3299.19 mV) | §3.5, Q11 |
| Ideal SNR = 6.02·N + 1.76 dB (8/12/16 bit: 49.92/74.0/98.08); ENOB = (SINAD−1.76)/6.02 | §3.5, Q11 |
| DAC = zero-order hold → reconstruction filter; timer-triggered sampling for low jitter | §3.5, Q12 |
| DMA ping-pong: half period = half_samples / fs (160 @8 kHz = 20 ms) | §3.6, Q13 |
| CPU deadline = one half-period after the half-ready IRQ; else overrun | §3.6, Q13 |
| DmaRing: overrun counted before the first sample of a half the CPU still holds | §3.6 |
| DMA removes per-sample overhead, not the deadline; mind cache and bus contention | §3.6, Q15 |
| 48 kHz: 256-sample half = 5.33 ms | §3.6, Q14 |
