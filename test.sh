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

assert 0 0
assert 42 42

assert 7 '5+2'
assert 3 '5-2'

assert 10 '5* 2'
assert 3 '6 /2'

assert 5 '+5'
assert 10 ' --10'

assert 2 '10 /(2 + 3 )'
assert 11 '2 + ( 3 * 3 )'

echo OK
