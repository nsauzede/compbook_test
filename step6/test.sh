#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert_fail() {
    expected="$1"
    input="$2"

    actual=`./9cc "$input" 2>&1`

    if [ "x$actual" = "x$expected" ]; then
        echo "$input => expected failure"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 41 " 12 + 34 - 5 "
assert_fail "1+3++
     ^ It's not a number" "1+3++"
assert_fail "1 + foo + 5
    ^ I can't tokenize 'f'" "1 + foo + 5"
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 10 '-10+20'

echo OK
