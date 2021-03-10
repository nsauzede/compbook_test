#!/bin/bash
assert() {
    expected="$1";input="$2"
    ./9cc "$input" > tmp.v
    v -o tmp tmp.v && ./tmp
    actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 41 " 12 + 34 - 5 "
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 10 '-10+20'
assert 1 '2<3'
assert 0 '3<2'
assert 1 '1<=2'
assert 1 '2<=2'
assert 0 '3<=2'
assert 1 '5>2'
assert 0 '2>2'
assert 1 '5>=2'
assert 1 '5>=5'
assert 0 '5>=6'
assert 1 '7==7'
assert 0 '7==8'
assert 1 '8!=7'
assert 0 '8!=8'
