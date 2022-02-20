#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
void alloc4(int**p,int a1, int a2, int a3, int a4){
*p=malloc(4*sizeof(a1));
(*p)[0]=a1;(*p)[1]=a2;(*p)[2]=a3;(*p)[3]=a4;
}
EOF
assert() {
    expected="$1";input="$2"
    echo "Testing input=[$input].."
    echo "$input" > tmp.c.txt
    #cc -static -o tmp tmp.s tmp2.o
    
    gcc -x c -O0 -fno-stack-protector -o tmp.gcc.s tmp.c.txt -S ; gcc -x c -o tmp.o tmp.c.txt -c ; gcc -static -o tmp tmp.o tmp2.o ; ./tmp ; actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "GCC $input => $actual"
    else
        echo "GCC $input => [$expected] expected, but got [$actual]"
        exit 1
    fi

    tcc -x c -o tmp tmp.c.txt ; ./tmp ; actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "TCC $input => $actual"
    else
        echo "TCC $input => [$expected] expected, but got [$actual]"
        exit 1
    fi

    ./chibicc "$input" > tmp.s || exit $? && gcc -static -o tmp tmp.s tmp2.o && ./tmp && actual="$?"
    # gdb -q -nx -ex r --args ./chibicc "$input"
    if [ "$actual" = "$expected" ]; then
        echo "CHI $input => $actual"
    else
        echo "CHI $input => [$expected] expected, but got [$actual]"
        exit 1
    fi
}

if test "x$1" != "x"; then
if test "x$2" != "x"; then
echo "will do: assert $*"
assert $1 "$2"
exit $?
else
echo "Usage: $0 <assert_ret_code> <C code>"
exit 1
fi
fi

#################

assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() { return  12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return -10+20; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'

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

assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'
assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
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
assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'
assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
# assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
assert 5 'int main() { int x=3; int y=5; int z=7; if (&y > &z) return *(&z+1); else return *(&x+1); }'
# assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 3 'int main() { int z=7; int x=3; int y=5; if (&y > &x) return *(&y-1); else return *(&z-1);}'
# assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
assert 5 'int main() { int x=3; int y=5; int z = 7; if (&y > &x) return *(&x-(-1)); else return *(&z-(-1)); }'
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
assert 4 'int main() { int *x; return sizeof(*x); }'
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

assert 12 'int main(){long x[2];x[0]=8031924123371070824;x[1]=174353522;return printf(x);}'
# 8031924123371070501=0x6f77206f6c6c6425 %=25 l=6c d=64
assert 13 'int main(){long x[2];x[0]=8031924123371070501;x[1]=174353522;return printf(x,123);}'

assert 42 'int main(){long x[2];x[0]=8031924123371070501;x[1]=174353522;printf(x,&x);return 42;}'
assert 33 'int main(){int x,y;x=33;y=5;int *z=&y+1;return *z;}'
assert 8 'int main(){int *p;alloc4(&p,1,2,4,8);int *q;q=p+3;return *q;}'

assert 42 'int main(){long x[2];x[0]=8031924123371070501;x[1]=174353522;printf(x,sizeof main);return 42;}'

assert 1 'int main(){return sizeof main;}'
assert 8 'int main(){return sizeof&main;}'
assert 4 'int main(){return sizeof 0;}'
assert 8 'int main(){return sizeof sizeof 0;}'

assert 3 'int main(){int a[2];*a=1;*(a+1)=2;int*p;p=a;return *p+*(p+1);}'
assert 42 'int main(){int a[4];a[3]=42;a[0]=0;return 3[a];}'

assert 42 'int x;int main(){x=3;foo();return x;}int foo(){x=42;}'

assert 4 'int x;int main(){return sizeof x;}'
assert 1 'char x;int main(){return sizeof x;}'
# 8031924123371070245=0x6f77206f6c6c6325 %=25 c=63
assert 42 'int main(){char c=65;long x[2];x[0]=8031924123371070245;x[1]=174353522;printf(x,c);return 42;}'
assert 65 'int main(){char x=65;return x;}'
#assert 42 "int main(){char c='B';long x[2];x[0]=8031924123371070245;x[1]=174353522;printf(x,c);return 42;}"

assert 42 'int main(){char*s="hellon";printf(s);return 42;}'
assert 42 'int main(){char*fmt="hello%c";char n=10;printf(fmt,n);return 42;}'
assert 42 'int main(){char*fmt="hello%c";printf(fmt,10);return 42;}'
assert 42 'int main(){char*fmt="hello %s%c";char*world="world";printf(fmt,world,10);return 42;}'

echo OK
