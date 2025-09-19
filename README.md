# Order Book Simulator



A basic C++ implementation of a limit order book with price-time priority matching, demonstrating how two order types, GTC (Good Till Cancelled) and FOK (Fill or Kill), interact in a matching engine with order lifecycle management (create, modify, cancel).  Simulated input is provided to illustrate usage.



## Compilation


build
```bash
make
```

clean
```bash
make clean
```

rebuild
```bash
make rebuild
```


## Usage



### Interactive Mode

```bash

./orderbook

```

Provides menu-driven interface for manual order entry and testing.



### CSV Batch Processing

```bash

./orderbook test_small.csv    # 10 orders - basic functionality

./orderbook test_medium.csv   # 100 orders - realistic scenarios

./orderbook test_large.csv    # 700+ orders - complex patterns

```



### CSV Format

```

action,order_id,side,type,price,quantity

CREATE,1001,BUY,GTC,95,100

MODIFY,1002,SELL,FOK,96,50

CANCEL,1007

```



## Remarks

A primary limitation is that the code supports only limited order types, GTC and FOK.  It would be interesting to add support for other order types (IOC, immediate or cancel, and market orders), partial fills for FOK orders, and (simulated) market data.