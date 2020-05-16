#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int add(int x, int y) { return x+y; }
int add6(int x, int y, int z, int l, int m, int n) { return x+y+z+l+m+n; }
EOF
assert() {
  expected="$1"
  input="$2"

  ./tcc "$input" > tmp.s
  gcc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 'int main(){ return 0; }'
assert 42 'int main(){ return 42;}'

assert 7 'int main() { return 5+2; }'
assert 3 'int main() { return 5-2; }'

assert 10 'int main() { return 5* 2; }'
assert 3 'int main() { return 6 /2; }'

assert 5 'int main() { return +5; }'
assert 10 'int main() { return --10; }'

assert 2 'int main() { return 10 /(2 + 3 ); }'
assert 11 'int main() { return 2 + ( 3 * 3 );}';

assert 1 'int main() { return 0 == 0; }'
assert 0 'int main() { return 0 == 1; }'
assert 0 'int main() { return 0 != 0; }'
assert 1 'int main() { return 0 != 1; }'
assert 1 'int main() { return 0 < 1; }'
assert 0 'int main() { return 0 < 0; }'
assert 1 'int main() { return 0 <= 0; }'
assert 0 'int main() { return 1 <= 0; }'
assert 0 'int main() { return 0 > 0; }'
assert 1 'int main() { return 1 > 0; }'
assert 1 'int main() { return 0 >= 0; }'
assert 0 'int main() { return 0 >= 1; }'

assert 26 'int main() { return 13 * 2; }'

assert 1 'int main() { 0; return 1; }'
assert 0 'int main() { 1; return 0; }'

assert 2 'int main() { 1; return 2; 3; }'

assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'

assert 4 'int main() { int a=3; a = a + 1; return a; }'

assert 3 'int main() { int ii = 3; return ii; }'
assert 12 'int main() { int ii = 3; int jj = 4; return ii * jj; }'

assert 1 'int main() { return 1; return 2; }'

assert 1 'int main() { if (1) return 1; return 2; }'
assert 2 'int main() { if (0) return 1; return 2; }'

assert 1 'int main() { if (1) return 1; else return 2; return 3; }'
assert 2 'int main() { if (0) return 1; else return 2; }'

assert 2 'int main() { if (0>1) return 1; else return 2; }'

assert 3 'int main() { {1; {2;} return 3;} }'

assert 1 'int main() { return 1; {return 2;} return 3; }'
assert 1 'int main() { {return 1;} return 2; return 3; }'

assert 3 'int main() { return ret3(); }'
assert 6 'int main() { return ret3() * 2; }'

assert 3 'int main() { return add(1, 2); }'
assert 21 'int main() { return add6(1, 2, 3, 4, 5, 6); }'

assert 3 'int main() { int i = 3; while(0) 0; return i; }'
assert 10 'int main() { int i = 3; while(i<10) i = i + 1; return i; }'

assert 8 'int main() { int i = 6; return sizeof(i); }'
assert 8 'int g; int main() { return sizeof(g); }'
assert 1 'int main() { char ca[4]; return sizeof(ca); }'

assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int y = &x; int z = &y; return **z; }'
assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'

assert 0 'int x; int main() { return 0; }'
assert 0 'int x; int main() { return x; }'
assert 2 'int x; int main() { x = 2; return x; }'
assert 0 'int x; int y; int main() { y = 2; return x; }'
assert 2 'int x; int y; int main() { y = 2; return y; }'

assert 1 'int main() { char c = 1; return c; }'
assert 1 'int main() { int i = 10; char c = 1; return c; }'
assert 2 'int main() { int i = 10; char c = 1; char h = 2; return h; }'

assert 4 'int main() { return ret4(); } int ret4() { return 4; }'
assert 12 'int main() { return ret4() * 3; } int ret4() { return 4; }'
assert 7 'int main() { return add2(3,4); } int add2(int x,int y) { return x+y; }'
assert 2 'int main() { return sub2(3,1); } int sub2(int x,int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 0 'int main() { return ({ 0; }); }'
assert 3 'int main() { return ({ int i = 3; i; }); }'
assert 2 'int main() { return ({ 0; 1; 2; }); }'

assert 0 'int main() { int x[2]; return 0; }'

assert 1 'int main() { int x[2]; *x=1; return *x; }'
assert 2 'int main() { int x[2]; *x=1; *(x+1)=2; *(x+2)=3; return *(x+1); }'
assert 3 'int main() { int x[2]; *x=1; *(x+1)=2; *(x+2)=3; return *(x+2); }'

assert 1 'int main() { char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; return *ca; }'
assert 2 'int main() { char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; return *(ca+1); }'
assert 3 'int main() { char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; return *(ca+2); }'

assert 2 'int main() { int x[2]; *(1+x)=2; return *(x+1); }'
assert 1 'int main() { char ca[2]; *(ca+1)=1; return *(1+ca); }'

assert 1 'int main() { int x[2]; x[0]=1; return x[0]; }'
assert 2 'int main() { int x[2]; x[0]=1; x[1]=2; x[2]=3; return x[1]; }'
assert 3 'int main() { int x[2]; x[0]=1; x[1]=2; x[2]=3; return x[2]; }'

assert 1 'int main() { char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; return ca[0]; }'
assert 2 'int main() { char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; return ca[1]; }'
assert 3 'int main() { char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; return ca[2]; }'

assert 2 'int main() { int x[1]; *(x+0)=1; x[1]=2; return *(x+1); }'

assert 97 "int main() { char c = 'a'; return c; }"
assert 98 "int main() { char c = 'b'; return c; }"
assert 99 "int main() { char c = 'c'; return c; }"

assert 2 'int main() { /* return 1; */ return 2; }'
assert 2 'int main() { // return 1;
return 2; }'

assert 0 'int main() { char *s = "test"; return 0; }'

echo OK
