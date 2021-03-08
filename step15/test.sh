#!/bin/bash
assert() {
    expected="$1";input="$2"
    ./9cc "$input" > tmp.s ; cc -o tmp tmp.s ; ./tmp;actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => [$expected] expected, but got [$actual]"
        exit 1
    fi
}
# assert 42 'main(){j=0;i=i+1;j=j+2;}'
# assert 10 'foo(){j=0;for(;j!=2;){j=j+1;}return 2;}'
assert 42 'main(){return 42;}'
# assert 6 'main(){foo2(i=42);return 0;}'
# assert 6 'foo();0;'
PRINT_NODES=1 assert 6 'foo(){i=0;j=0;for(;i<=10;i=i+1){j=j+1;i=i+1;}return j;}'
assert 22 'j=0;for(i=0;i<=10;i=i+1)j=j+2;j;'
assert 22 'j=0;i=0;while(i<11) j=2*(i+1)+(i=i+1)-i;j;'
assert 66 'if (1) 66; else 77;';assert 77 'if (0) 66; else 77;'
assert 0 '0;';assert 42 '42;';assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;";assert 47 '5+6*7;'
assert 15 '5*(9-6);';assert 4 '(3+5)/2;';assert 10 '-10+20;'
assert 1 '2<3;';assert 0 '3<2;'
assert 1 '1<=2;';assert 1 '2<=2;';assert 0 '3<=2;'
assert 1 '5>2;';assert 0 '2>2;';assert 1 '5>=2;'
assert 1 '5>=5;';assert 0 '5>=6;';assert 1 '7==7;';
assert 0 '7==8;';assert 1 '8!=7;';assert 0 '8!=8;'
assert 42 '42;';assert 42 'a=2;43;a+40;';assert 43 '41;43;'
assert 14 'a = 3;b = 5 * 6 - 8;a + b / 2;// some comment'
assert 2 'fee=2;fee;';assert 2 'fee=2;foo=3;fee;'
assert 6 'foo = 1;bar = 2 + 3; foo + bar; // 6を返す'
assert 18 'Foo1 = 2;Foo2 = 20; Foo2 - Foo1;'
assert 9 'foo = 2;bar = 2 * 3;return 1+(baz = foo + bar);'
