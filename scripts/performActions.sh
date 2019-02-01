#!/bin/bash

cleos push action salescon init '["seller", "buyer", "intermed"]' -p seller@active
cleos push action salescon setitem '["asd", 100000]' -p seller@active
cleos push action eosio.token transfer '[ "buyer", "salescon", "10.0000 SYS", "m" ]' -p buyer@active
cleos push action salescon itemreceived '[]' -p buyer@active
