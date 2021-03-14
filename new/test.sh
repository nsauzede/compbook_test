#!/bin/bash
assert() {
    expected="$1";input="$2"
    echo "Testing input=[$input].."
    ./9cc "$input" > tmp.s ; cc -o tmp tmp.s ; ./tmp;actual="$?"
    # gdb -q -nx -ex r --args ./9cc "$input"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => [$expected] expected, but got [$actual]"
        exit 1
    fi
}
# PRINT_TOKENS=1 assert 42 'int main(){1;2;3;}'
# assert 6 'int main(){foo2()return 42;bar{i=0;return foo2(i=42);}'
# assert 6 'int main(){bar(){i=0;return foo2(i=42);}'
# assert 6 'int main(){foo2(i=42);0;}'
# assert 6 'int main(){foo();0;}'
# assert 10 'int main(){return 10;}'
assert 1 'int main(){return 0<1;}'
assert 6 'int main(){int j=0, i;for(i=0;i<=10;i=i+1){int j=j+1;i=i+1;}return j;}'
assert 22 'int main(){int j=0,i;for(i=0;i<=10;i=i+1)j=j+2;return j;}'
assert 22 'int main(){int j=0,i=0;while(i<11) {j=2*(i+1);i=i+1;}return j;}'
assert 66 'int main(){if (1) 66; else 77;}';assert 77 'int main(){if (0) 66; else 77;}'
assert 0 'int main(){0;}';assert 42 'int main(){42;}';assert 21 'int main(){5+20-4;}'
assert 41 'int main(){ 12 + 34 - 5 ;}';assert 47 'int main(){5+6*7;}'
assert 15 'int main(){5*(9-6);}';assert 4 'int main(){(3+5)/2;}';assert 10 'int main(){-10+20;}'
assert 1 'int main(){2<3;}';assert 0 'int main(){3<2;}'
assert 1 'int main(){1<=2;}';assert 1 'int main(){2<=2;}';assert 0 'int main(){3<=2;}'
assert 1 'int main(){5>2;}';assert 0 'int main(){2>2;}';assert 1 'int main(){5>=2;}'
assert 1 'int main(){5>=5;}';assert 0 'int main(){5>=6;}';assert 1 'int main(){7==7;}';
assert 0 'int main(){7==8;}';assert 1 'int main(){8!=7;}';assert 0 'int main(){8!=8;}'
assert 42 'int main(){42;}';assert 42 'int main(){int a=2;43;a+40;}';assert 43 'int main(){41;43;}'
assert 14 'int main(){int a = 3,b = 5 * 6 - 8;a + b / 2;}// some comment'
assert 2 'int main(){int fee=2;fee;}';assert 2 'int main(){int fee=2;int foo=3;fee;}'
assert 6 'int main(){int foo = 1;int bar = 2 + 3; foo + bar;} // 6を返す'
assert 18 'int main(){int Foo1 = 2;int Foo2 = 20; Foo2 - Foo1;}'
assert 9 'int main(){int foo = 2;int bar = 2 * 3;int baz;return 1+(baz = foo + bar);}'
