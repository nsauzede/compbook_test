#!/bin/bash
assert() {
    expected="$1";input="$2"
    aPRINT_TOKENS=1 GEN_V=1 ./9cc "$input" > tmp.v
    v -o tmp tmp.v ; ./tmp;actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => [$expected] expected, but got [$actual]"
        exit 1
    fi
}
# assert 6 '{foo2(i=42);0;}'
# assert 6 '{foo();0;}'
assert 6 '{j=0;for(i=0;i<=10;i=i+1){j=j+1;i=i+1;42;}return j;}'
assert 22 '{j=0;for(i=0;i<=10;i=i+1)j=j+2;return j;}'
assert 22 '{j=0;i=0;while(i<11){j=j+2;i=i+1;}return j;}'
assert 66 '{if (1) return 66; else return 77;}';assert 77 '{if (0) return 66; else return 77;}'
assert 14 '{a = 3;b = 5 * 6 - 8;return a + b / 2;}// some comment'
assert 2 '{fee=2;return fee;}';assert 2 '{fee=2;foo=3;return fee;}'
assert 6 '{foo = 1;bar = 2 + 3; return foo + bar;} // 6を返す'
assert 18 '{Foo1 = 2;Foo2 = 20; return Foo2 - Foo1;}'
assert 9 '{foo = 2;bar = 2 * 3;baz = foo + bar;return 1+baz;}'
assert 0 '{return 0;}';assert 42 '{return 42;}';assert 21 '{return 5+20-4;}'
assert 41 '{return  12 + 34 - 5 ;}';assert 47 '{return 5+6*7;}'
assert 15 '{return 5*(9-6);}';assert 4 '{return (3+5)/2;}';assert 10 '{return -10+20;}'
assert 1 '{return 2<3;}';assert 0 '{return 3<2;}'
assert 1 '{return 1<=2;}';assert 1 '{return 2<=2;}';assert 0 '{return 3<=2;}'
assert 1 '{return 5>2;}';assert 0 '{return 2>2;}';assert 1 '{return 5>=2;}'
assert 1 '{return 5>=5;}';assert 0 '{return 5>=6;}';assert 1 '{return 7==7;}'
assert 0 '{return 7==8;}';assert 1 '{return 8!=7;}';assert 0 '{return 8!=8;}'
#assert 42 '{a=2;43;a+40;}';assert 43 '41;43;}'