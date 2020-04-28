#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./tcc "$input" > tmp.s
  gcc -static -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 'return 0;'
assert 42 'return 42;'

assert 7 'return 5+2;'
assert 3 'return 5-2;'

assert 10 'return 5* 2;'
assert 3 'return 6 /2;'

assert 5 'return +5;'
assert 10 'return  --10;'

assert 2 'return 10 /(2 + 3 );'
assert 11 'return 2 + ( 3 * 3 );';

assert 1 'return 0 == 0;'
assert 0 'return 0 == 1;'
assert 0 'return 0 != 0;'
assert 1 'return 0 != 1;'
assert 1 'return 0 < 1;'
assert 0 'return 0 < 0;'
assert 1 'return 0 <= 0;'
assert 0 'return 1 <= 0;'
assert 0 'return 0 > 0;'
assert 1 'return 1 > 0;'
assert 1 'return 0 >= 0;'
assert 0 'return 0 >= 1;'

assert 1 '0; return 1;'
assert 0 '1; return 0;'

assert 2 '1; return 2; 3;'

assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'

echo OK
