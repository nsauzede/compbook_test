#!/bin/bash
cat <<EOF2 > tmp2.v
fn cf_ret3() int { return 3 }
fn cf_ret5() int { return 5 }
fn cf_add(x int, y int) int { return x+y }
fn cf_sub(x int, y int) int { return x-y }
fn cf_add6(a int, b int, c int, d int, e int, f int) int {
  return a+b+c+d+e+f
}
EOF2
assert() {
    expected="$1";input="$2"
    # aPRINT_TOKENS=1 GEN_V=1 gdb -q -nx -ex r --args ./chibicc "$input"
    # aPRINT_TOKENS=1 GEN_V=1 ./chibicc "$input" > tmp.v && \
    cat tmp2.v > tmp.v && aPRINT_TOKENS=1 GEN_V=1 ./chibicc "$input" >> tmp.v && \
    v --enable-globals -translated -o tmp tmp.v && ./tmp;actual="$?"
    # v --enable-globals -translated -o tmp.c tmp.v && gcc -o tmp tmp.c tmp2.o && ./tmp;actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => [$expected] expected, but got [$actual]"
        exit 1
    fi
}

assert 7 'int main() { int x=3; int y=5; int z=9; if (&y > &x) *(&x+1)=7; else *(&z+1)=7; return y; }'
PRINT_AST=1 assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'
assert 0 'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; } '
PRINT_AST=1 assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
#########

assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() { return  12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return -10+20; }'
# assert 10 'int main() { return - -10; }'
# assert 10 'int main() { return - - +10; }'

assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'

assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'

assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 3 'int main() { int a; a=3; return a; }'
assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'

# assert 1 'int main() { return 1; 2; 3; }'
# assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'
# assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
assert 3 'int main() { int foo=3; return foo; }'
assert 8 'int main() { int foo123=3; int bar=5; return foo123+bar; }'

assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'

assert 55 'int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) return 3; return 5; }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'

assert 3 'int main() { {1; {2;} return 3;} }'
assert 5 'int main() { ;;; return 5; }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'
assert 55 'int main() { int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
# assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
assert 5 'int main() { int x=3; int y=5; int z=7; if (&y > &z) return *(&z+1); else return *(&x+1); }'
# assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 3 'int main() { int z=7; int x=3; int y=5; if (&y > &x) return *(&y-1); else return *(&z-1);}'
# assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
assert 5 'int main() { int x=3; int y=5; int z = 7; if (&y > &x) return *(&x-(-1)); else return *(&z-(-1)); }'
assert 5 'int main() { int x=3; int *y=&x; *y=5; return x; }'
# assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
assert 7 'int main() { int x=3; int y=5; int z=9; if (&y > &x) *(&x+1)=7; else *(&z+1)=7; return y; }'
# assert 7 'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }'
assert 7 'int main() { int z=9; int x=3; int y=5; if (&z > &x) *(&z-2+1)=7; else *(&y-2+1)=7; return x; }'
assert 5 'int main() { int x=3; return (&x+2)-&x+3; }'
assert 8 'int main() { int x, y; x=3; y=5; return x+y; }'
assert 8 'int main() { int x=3, y=5; return x+y; }'

assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'

assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
assert 1 'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }'
assert 2 'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }'
assert 3 'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }'
assert 4 'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
assert 5 'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }'

assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'

assert 8 'int main() { long x; return sizeof(x); }'
assert 8 'int main() { long x; return sizeof x; }'
assert 8 'int main() { int *x; return sizeof(x); }'
assert 32 'int main() { long x[4]; return sizeof(x); }'
assert 96 'int main() { long x[3][4]; return sizeof(x); }'
assert 32 'int main() { long x[3][4]; return sizeof(*x); }'
assert 8 'int main() { long x[3][4]; return sizeof(**x); }'
assert 9 'int main() { long x[3][4]; return sizeof(**x) + 1; }'
assert 9 'int main() { long x[3][4]; return sizeof **x + 1; }'
assert 8 'int main() { long x[3][4]; return sizeof(**x + 1); }'
assert 8 'int main() { long x=1; return sizeof(x=2); }'
assert 1 'int main() { int x=1; sizeof(x=2); return x; }'

assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x=3; return x; }'
assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
assert 7 'int x, y; int main() { x=3; y=4; return x+y; }'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

assert 8 'long x; int main() { return sizeof(x); }'
assert 32 'long x[4]; int main() { return sizeof(x); }'
echo OK
exit 0

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