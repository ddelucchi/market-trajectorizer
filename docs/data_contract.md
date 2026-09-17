# Data Contract

## Canonical OHLCV
A `CandleColumns` is a struct-of-arrays:

| field | type        | semantics                                |
|-------|-------------|------------------------------------------|
| ts    | int64 (ns)  | UNIX nanoseconds, UTC, strictly increasing |
| open  | double      | first trade price in bar                 |
| high  | double      | max trade price in bar                   |
| low   | double      | min trade price in bar                   |
| close | double      | last trade price in bar                  |
| volume| double      | sum of trade sizes (units of `lot_size`) |

Invariants enforced by `canonicalize`:
1. `low <= min(open, close) <= max(open, close) <= high`
2. `ts` strictly increasing after stable sort + dedupe
3. `volume >= 0`, finite
4. all prices > 0, finite

## On-disk binary format (`save_canonical` / `load_canonical`)
```
magic   : char[8]  = "MTCAND01"
n       : uint64
ts      : int64 [n]
open    : f64   [n]
high    : f64   [n]
low     : f64   [n]
close   : f64   [n]
volume  : f64   [n]
```
Little-endian, no padding.
