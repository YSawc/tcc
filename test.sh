#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int retThree() { return 3; }
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

assert 0 'main(){ return 0; }'
assert 42 'main(){ return 42;}'

assert 7 'main() { return 5+2; }'
assert 3 'main() { return 5-2; }'

assert 10 'main() { return 5* 2; }'
assert 3 'main() { return 6 /2; }'

assert 5 'main() { return +5; }'
assert 10 'main() { return --10; }'

assert 2 'main() { return 10 /(2 + 3 ); }'
assert 11 'main() { return 2 + ( 3 * 3 );}';

assert 1 'main() { return 0 == 0; }'
assert 0 'main() { return 0 == 1; }'
assert 0 'main() { return 0 != 0; }'
assert 1 'main() { return 0 != 1; }'
assert 1 'main() { return 0 < 1; }'
assert 0 'main() { return 0 < 0; }'
assert 1 'main() { return 0 <= 0; }'
assert 0 'main() { return 1 <= 0; }'
assert 0 'main() { return 0 > 0; }'
assert 1 'main() { return 1 > 0; }'
assert 1 'main() { return 0 >= 0; }'
assert 0 'main() { return 0 >= 1; }'

assert 26 'main() { return 13 * 2; }'

assert 1 'main() { 0; return 1; }'
assert 0 'main() { 1; return 0; }'

assert 2 'main() { 1; return 2; 3; }'

assert 3 'main() { a=3; return a; }'
assert 8 'main() { a=3; z=5; return a+z; }'

assert 3 'main() { ii = 3; return ii; }'
assert 12 'main() { ii = 3; jj = 4; return ii * jj; }'

assert 1 'main() { return 1; return 2; }'

assert 1 'main() { if (1) return 1; return 2; }'
assert 2 'main() { if (0) return 1; return 2; }'

assert 1 'main() { if (1) return 1; else return 2; return 3; }'
assert 2 'main() { if (0) return 1; else return 2; }'

assert 2 'main() { if (0) return 1; else return 2; }'

assert 3 'main() { {1; {2;} return 3;} }'

assert 1 'main() { return 1; {return 2;} return 3; }'
assert 1 'main() { {return 1;} return 2; return 3; }'

assert 3 'main() { return retThree(); }'
assert 6 'main() { return retThree() * 2; }'

assert 3 'main() { i = 3; while(0) 0; return i; }'

echo OK
