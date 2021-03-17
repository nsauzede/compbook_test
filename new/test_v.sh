#!/bin/bash
assert() {
    expected="$1";input="$2"
    # aPRINT_TOKENS=1 GEN_V=1 gdb -q -nx -ex r --args ./chibicc "$input"
    aPRINT_TOKENS=1 GEN_V=1 ./chibicc "$input" > tmp.v && \
    v -enable-globals -translated -o tmp tmp.v && ./tmp;actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => [$expected] expected, but got [$actual]"
        exit 1
    fi
}

assert 3 'int main() { int x=3; return *&x; }'
assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'

assert 42 'int g;int foo(){g = 6*7;return 0;}int main(){foo();return g;}'
assert 42 'int foo(){return 42;}int main(){return foo();}'
assert 21 'int fibonacci(int n){if (n<=1)return n;return fibonacci(n-1)+fibonacci(n-2);}int main(){return fibonacci(8);}'
assert 1 'int main(){return 0<1;}'
assert 6 'int main(){int j=0, i;for(i=0;i<=10;i=i+1){j=j+1;i=i+1;}return j;}'
assert 22 'int main(){int j=0,i;for(i=0;i<=10;i=i+1)j=j+2;return j;}'
assert 22 'int main(){int j=0,i=0;while(i<11) {j=2*(i+1);i=i+1;}return j;}'
assert 66 'int main(){if (1) return 66; else return 77;}';assert 77 'int main(){if (0) return 66; else return 77;}'
assert 0 'int main(){return 0;}';assert 42 'int main(){return 42;}';assert 21 'int main(){return 5+20-4;}'
assert 41 'int main(){return  12 + 34 - 5 ;}';assert 47 'int main(){return 5+6*7;}'
assert 15 'int main(){return 5*(9-6);}';assert 4 'int main(){return (3+5)/2;}';assert 10 'int main(){return -10+20;}'
assert 1 'int main(){return 2<3;}';assert 0 'int main(){return 3<2;}'
assert 1 'int main(){return 1<=2;}';assert 1 'int main(){return 2<=2;}';assert 0 'int main(){return 3<=2;}'
assert 1 'int main(){return 5>2;}';assert 0 'int main(){return 2>2;}';assert 1 'int main(){return 5>=2;}'
assert 1 'int main(){return 5>=5;}';assert 0 'int main(){return 5>=6;}';assert 1 'int main(){return 7==7;}';
assert 0 'int main(){return 7==8;}';assert 1 'int main(){return 8!=7;}';assert 0 'int main(){return 8!=8;}'
assert 42 'int main(){return 42;}';assert 42 'int main(){int a=2;43;return a+40;}';assert 43 'int main(){41;return 43;}'
assert 14 'int main(){int a = 3,b = 5 * 6 - 8;return a + b / 2;}// some comment'
assert 2 'int main(){int fee=2;return fee;}';assert 2 'int main(){int fee=2;int foo=3;return fee;}'
assert 6 'int main(){int foo = 1;int bar = 2 + 3; return foo + bar;} // 6を返す'
assert 18 'int main(){int Foo1 = 2;int Foo2 = 20; return Foo2 - Foo1;}'
assert 9 'int main(){int foo = 2;int bar = 2 * 3;int baz = foo + bar;return 1+(baz);}'
echo "OK"