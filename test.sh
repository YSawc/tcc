#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int retThree() { return 3; }
int add(int x, int y) { return x+y; }
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

assert 3 'int main() { return retThree(); }'
assert 6 'int main() { return retThree() * 2; }'

assert 3 'int main() { return add(1, 2); }'

assert 3 'int main() { int i = 3; while(0) 0; return i; }'
assert 10 'int main() { int i = 3; while(i<10) i = i + 1; return i; }'

assert 8 'int main() { int i = 6; return sizeof(i); }'

assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int y = &x; int z = &y; return **z; }'
assert 3 'int main() { int x=3; int y=5; return *(&y-8); }'
assert 5 'int main() { int x=3; int y=5; return *(&x+8); }'

assert 0 'int x; int main() { return 0; }'
assert 0 'int x; int main() { return x; }'
assert 2 'int x; int main() { x = 2; return x; }'
assert 0 'int x; int y; int main() { y = 2; return x; }'
assert 2 'int x; int y; int main() { y = 2; return y; }'

echo OK
