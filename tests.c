char gc1;
char gc2;

int gi1;
int gi2;

int printf(const char *s, ...);
void exit(int i);

int vd();
int argf(int a, int b, int c);

int ret3() {
	return 3;
}

int add(int x, int y) {
	return x+y;
}

int add6(int x, int y, int z, int l, int m, int n) {
	return x+y+z+l+m+n;
}

int fib(int x) {
	if (x<=1)
		return 1;
	return fib(x-1) + fib(x-2);
}

int assert(int expected, int actual, char *code) {
	if (expected == actual) {
		printf("%s => %d\n", code, actual);
	} else {
		printf("%s => %d expected but got %d\n", code, expected, actual);
		exit(1);
	}
	return 0;
}

int main() {
	assert(0, 0, "0");
	assert(42, 42, "42");
	assert(7, 5+2, "5+2");
	assert(3, 5-2, "5-2");
	assert(10, 5*2, "5*2");
	assert(3, 6 /2, "6 /2");
	assert(5, +5, "+5");
	assert(2, 10 /(2 + 3 ), "10 /(2 + 3 )");
	assert(11, 2 + ( 3 * 3 ), "2 + ( 3 * 3 )");
	assert(1, ({ 0 == 0; }), "0 == 0");
	assert(0, ({ 0 == 1; }), "0 == 1");
	assert(0, ({ 0 != 0; }), "0 != 0");
	assert(1, ({ 0 != 1; }), "0 != 1");
	assert(1, ({ 0 < 1; }), "0 < 1");
	assert(0, ({ 0 < 0; }), "0 < 0");
	assert(1, ({ 0 <= 0; }), "0 <= 0");
	assert(0, ({ 1 <= 0; }), "1 <= 0");
	assert(0, ({ 0 > 0; }), "0 > 0");
	assert(1, ({ 1 > 0; }), "1 > 0");
	assert(1, ({ 0 >= 0; }), "0 >= 0");
	assert(0, ({ 0 >= 1; }), "0 >= 1");
	assert(26, 13 * 2, "13 * 2");
	assert(1, ({0; 1;}), "0; 1;");
	assert(0, ({1; 0;}), "1; 0;");
	assert(3, ({1; 2; 3;}), "1; 2; 3;");
	assert(3, ({int a=3; a;}), "int a=3; a;");
	assert(8, ({ int a=3; int z=5; a+z; }), "int a=3; int z=5; a+z;");
	assert(4, ({ int a=3; a = a + 1; a; }), "int a=3; a = a + 1; a;");
	assert(3, ({ int ii = 3; ii; }), "int ii = 3; ii;");
	assert(12, ({ int ii = 3; int jj = 4; ii * jj; }), "int ii = 3; int jj = 4; ii * jj;");
	assert(1, ({ int x; if (1) x=1; else x=2; x; }), "int x; if (1) x=1; else x=2; x;");
	assert(3, ({ 1; {2;} 3; }), "1; {2;} 3;");
	assert(3, ret3(), "ret3()");
	assert(6, ret3() * 2, "ret3() * 2");
	assert(3, add(1, 2), "add(1, 2)");
	assert(21, add6(1, 2, 3, 4, 5, 6), "add6(1, 2, 3, 4, 5, 6)");
	assert(3, ({ int i = 3; while(0) 0; i;}), "int i = 3; while(0) 0; i");
	assert(10, ({ int i = 3; while(i<10) i=i+1; i;}), "int i = 3; while(i<10) i=i+1; i;");
	assert(4, ({ int i = 6; sizeof(i); }), "int i = 6; sizeof(i);");
	assert(4, sizeof(gi1), "sizeof(gi1)");
	assert(1, ({ char ca[4]; sizeof(ca); }), "char ca[4]; sizeof(ca);");
	assert(3, ({ int x=3; *&x; }), "int x=3; *&x;");
	/* assert(3, ({ int x=3; int y = &x; int z = &y; **z; }), "int x=3; int y = &x; int z = &y; **z;"); */
	/* assert(3, ({ int x=3; int y = &x; int z = &y; **z; }), "int x=3; int y = &x; int z = &y; **z;"); */
	assert(3, ({ int x=3; int y=5; *(&y-1); }), "int x=3; int y=5; *(&y-1);");
	assert(5, ({ int x=3; int y=5; *(&x+1); }), "int x=3; int y=5; *(&x+1);");
	assert(6, ({ int i=3; char c=2; i*c; }), "int i=3; char c=2; i*c;");
	assert(0, gi1, "gi1");
	assert(2, ({ gi1 = 2; gi1; }), "gi1 = 2; gi1;"); /* FOR_INIT: */ gi1=0;
	assert(0, ({ gi2 = 2; gi1; }), "gi2 = 2; gi1;"); /* FOR_INIT: */ gi2=0;
	assert(2, ({ gi2 = 2; gi2; }), "gi2 = 2; gi2;"); /* FOR_INIT: */ gi2=0;
	assert(1, sizeof(gc1), "sizeof(gc1)");
	assert(1, gc1+1, "gc1+1");
	assert(2, ({ gc1 = 2; gc1; }), "gc1 = 2; gc1;"); /* FOR_INIT: */ gc1=0;
	assert(0, ({ gc2 = 2; gc1; }), "gc2 = 2; gc1;"); /* FOR_INIT: */ gc2=0;
	assert(1, ({ char c = 1; c; }), "char c = 1; c;");
	assert(1, ({ int i = 10; char c = 1; c; }), "int i = 10; char c = 1; c;");
	assert(2, ({ int i = 10; char c = 1; char h = 2; h; }), "int i = 10; char c = 1; char h = 2; h;");
	assert(55, fib(9), "fib(9)");
	assert(0, ({ int x[2]; 0; }), "int x[2]; 0;");
	assert(1, ({ int x[2]; *x=1; *x; }), "int x[2]; *x=1; *x;");
	assert(2, ({ int x[2]; *x=1; *(x+1)=2; *(x+2)=3; *(x+1); }), "int x[2]; *x=1; *(x+1)=2; *(x+2)=3; *(x+1);");
	assert(3, ({ int x[2]; *x=1; *(x+1)=2; *(x+2)=3; *(x+2); }), "int x[2]; *x=1; *(x+1)=2; *(x+2)=3; *(x+2);");
	assert(1, ({ char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; *ca; }), "char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; *ca;");
	assert(2, ({ char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; *(ca+1); }), "char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; *(ca+1);");
	assert(3, ({ char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; *(ca+2); }), "char ca[2]; *ca=1; *(ca+1)=2; *(ca+2)=3; *(ca+2);");
	assert(2, ({ int x[2]; *(1+x)=2; *(x+1); }), "int x[2]; *(1+x)=2; *(x+1);");
	assert(1, ({ char ca[2]; *(ca+1)=1; *(1+ca); }), "char ca[2]; *(ca+1)=1; *(1+ca);");
	assert(1, ({ int x[2]; x[0]=1; x[0]; }), "int x[2]; x[0]=1; x[0];");
	assert(3, ({ int x[2]; x[0]=1; x[1]=2; x[2]=3; x[2]; }), "int x[2]; x[0]=1; x[1]=2; x[2]=3; x[2];");
	assert(2, ({ int x[2]; x[0]=1; x[1]=2; x[2]=3; x[1]; }), "int x[2]; x[0]=1; x[1]=2; x[2]=3; x[1];");
	assert(2, ({ char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; ca[1]; }), "char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; ca[1];");
	assert(1, ({ char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; ca[0]; }), "char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; ca[0];");
	assert(3, ({ char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; ca[2]; }), "char ca[2]; ca[0]=1; ca[1]=2; ca[2]=3; ca[2];");
	assert(2, ({ int x[1]; *(x+0)=1; x[1]=2; *(x+1); }), "int x[1]; *(x+0)=1; x[1]=2; *(x+1);");
	assert(97, ({ char c = 'a'; c; }), "char c = 'a'; c;");
	assert(98, ({ char c = 'b'; c; }), "char c = 'b'; c;");
	assert(99, ({ char c = 'c'; c; }), "char c = 'c'; c;");
	assert(2, ({ /* return 1; */ 2; }), "/* 1; */ 2;");
	assert(2, ({ // 1;
	2; }), "//1\n2");
	assert(0, ({ char *s = "test"; 0; }), "char *s = \"test\"; 0;");
	assert(97, ({ char *s = "abc"; s[0]; }), "char *s = \"abc\"; s[0];");
	assert(98, ({ char *s = "abc"; s[1]; }), "char *s = \"abc\"; s[1];");
	assert(99, ({ char *s = "abc"; s[2]; }), "char *s = \"abc\"; s[2];");
	assert(3, ({ int x=2; { x=3; } x; }), "int x=2; { x=3; } x;");
	assert(2, ({ int x=2; { int x=3; } x; }), "int x=2; { int x=3; } x;");
	assert(2, ({ int x=2; { int x=3; } { int y=4; } x; }), "int x=2; { int x=3;} { int x=4; }");
	assert(3, ({ int i = 3; i; }), "int i = 3; i;");
	assert(1, ({ char c; sizeof(c); }), "char c; sizeof(c);");
	assert(7, ({ char *s="\a"; s[0]; }), "char *s=\"\\a\"; s[0];");
	assert(8, ({ char *s="\b"; s[0]; }), "char *s=\"\\b\"; s[0];");
	assert(9, ({ char *s="\t"; s[0]; }), "char *s=\"\\t\"; s[0];");
	assert(10, ({ char *s="\n"; s[0]; }), "char *s=\"\\n\"; s[0];");
	assert(11, ({ char *s="\v"; s[0]; }), "char *s=\"\\v\"; s[0];");
	assert(12, ({ char *s="\f"; s[0]; }), "char *s=\"\\f\"; s[0];");
	assert(13, ({ char *s="\r"; s[0]; }), "char *s=\"\\r\"; s[0];");
	assert(27, ({ char *s="\e"; s[0]; }), "char *s=\"\\e\"; s[0];");
	assert(99, ({ char *s = "\c"; s[0]; }), "char *s=\"\c\"; s[0];");
	assert(100, ({ char *s = "\d"; s[0]; }), "char *s=\"\d\"; s[0];");
	assert(103, ({ char *s = "\g"; s[0]; }), "char *s=\"\g\"; s[0];");
	assert(48, ({ char *s = "0"; s[0]; }), "char *s=\"0\"; s[0];");
	assert(7, ({ "\a"[0]; }), "\"\\a\"[0];");
	assert(8, ({ "\b"[0]; }), "\"\\b\"[0];");
	assert(7, ({ "\a\b"[0]; }), "\"\\a\\b\"[0];");
	assert(8, ({ "\a\b"[1]; }), "\"\\a\\b\"[1];");
	assert(97, ({ "a\bc"[0]; }), "\"a\\bc\"[0];");
	assert(8, ({ "a\bc"[1]; }), "\"a\\bc\"[1];");
	assert(99, ({ "a\bc"[2]; }), "\"a\\bc\"[2];");
	assert(0, ({ ({ 0; }); }), "({ 0; })");
	assert(2, ({ ({ 0; 1; 2; }); }), "({ 0; 1; 2; });");
	assert(3, ({ ({ 0; 1; 2; }); 3; }), "({ 0; 1; 2; }); 3;");
	assert(97, ({ char *s; s="abc"; s[0]; }), "char *s; s=\"abc\"; s[0];");
	assert(98, ({ char *s; s="abc"; s[1]; }), "char *s; s=\"abc\"; s[1];");
	assert(99, ({ char *s; s="abc"; s[2]; }), "char *s; s=\"abc\"; s[2];");
	assert(2, ({ struct { int x; int y; }s; s.x=2; s.y=3; s.x;}), "struct { int x; int y; }s; s.x=2; s.y=3; s.x;");
	assert(3, ({ struct { int x; int y; }s; s.x=2; s.y=3; s.y;}), "struct { int x; int y; }s; s.x=2; s.y=3; s.y;");
	assert(14, ({ struct { int x; int y; }s; s.x=2; s.y=3; struct { int l; int m; }n; n.l=4; n.m=5; s.x+s.y+n.l+n.m;}), "struct { int x; int y; }s; s.x=2; s.y=3; struct { int l; int m; }n; n.l=4; n.m=5; s.x+s.y+n.l+n.m;");
	assert(97, ({ struct { char c; char *s; }st; st.c='a'; st.s="bc"; st.c; }), "struct { char c; char *s; }st; st.c='a'; st.s=\"bc\"; st.c;");
	/* assert(98, ({ struct { char c; char *s; }st; st.c='a'; st.s="bc"; st.s[0]; }), "struct { char c; char *s; }st; st.c='a'; st.s=\"bc\"; st.s[0];"); */
	/* assert(99, ({ struct { char c; char *s; }st; st.c='a'; st.s="bc"; st.s[1]; }), "struct { char c; char *s; }st; st.c='a'; st.s=\"bc\"; st.s[1];"); */
	/* assert(98, ({ struct { int i; char *s; }st; st.i=2; st.s="bc"; st.s[0];}), "struct { char c; char *s; }st; st.c='a'; st.s=\"bc\"; st.c;"); */
	assert(97, ({ struct { int i; char c; }st; st.i=3; st.c='a'; st.c; }), "struct { char c; char *s; }st; st.c='a'; st.s=\"bc\"; st.c;");
	assert(8, ({ struct { int i; char c; }st; sizeof(st); }), "struct { int i; char c; }st; sizeof(st);");
	/* assert(24, ({ struct { int i; char c; char *s; }st; sizeof(st); }), "struct { int i; char c; char *s; }st; sizeof(st);"); */
	assert(2, ({ struct { int x; int y; }s; s.x=2; s->y=3; s->x;}), "struct { int x; int y; }s; s.x=2; s.y=3; s.x;");
	assert(1, ({ _Bool b = 1; b; }), "_Bool b = 1; b;");
	assert(0, ({ _Bool b = 0; b; }), "_Bool b = 0; b;");
	assert(1, ({ _Bool b = 3; b; }), "_Bool b = 3; b;");
	assert(1, ({ enum { l, m, n }; l; }), "enum { l, m, n }; l;");
	assert(2, ({ enum { l, m, n }; m; }), "enum { l, m, n }; m;");
	assert(3, ({ enum { l, m, n }; n; }), "enum { l, m, n }; n;");
	assert(4, ({ enum { l }; sizeof(l); }), "enum { l }; sizeof(l);");
	assert(1, sizeof(char), "sizeof(char)");
	assert(4, sizeof(int), "sizeof(int)");
	assert(4, sizeof(float), "sizeof(float)");
	assert(1, sizeof(_Bool), "sizeof(_Bool)");
	/* assert(0, sizeof(void), "sizeof(void)"); */
	assert(8, sizeof(double), "sizeof(double)");
	assert(8, sizeof(char *), "sizeof(char *)");
	assert(8, sizeof(int *), "sizeof(int *)");
	assert(8, sizeof(float *), "sizeof(float *)");
	assert(8, sizeof(_Bool *), "sizeof(_Bool *)");
	/* assert(8, sizeof(void *), "sizeof(void *)"); */
	assert(8, sizeof(double *), "sizeof(double *)");
	{ void *v; } printf("void *v; => ;\n");
	assert(-1, ~0, "~0");
	assert(-2, ~1, "~1");
	assert(-255, ~254, "~254");
	assert(-256, ~255, "~255");
	assert(-257, ~256, "~256");
	assert(1, !0, "!0");
	assert(0, !1, "!1");
	assert(0, !2, "!2");
	assert(1, ({ 0 ? 0 : 1; }), "0 ? 0 : 1;");
	assert(1, ({ 1 ? 1 : 0; }), "1 ? 1 : 0;");
	assert(0, ({ int i; i++; }), "int i; i++;");
	assert(1, ({ int i; i++; i; }), "int i; i++; i;");
	assert(1, ({ int i=1; i--; }), "int i=1; i--;");
	assert(0, ({ int i=1; i--; i; }), "int i=1; i--; i;");
	assert(3, ({ typedef int ti; ti i = 3; i; }), "typedef int ti; ti i = 3; i;");
	assert(1, ({ typedef char tc; sizeof(tc); }), "typedef char tc; sizeof(tc);");
	assert(4, ({ typedef int ti; sizeof(ti); }), "typedef int ti; sizeof(ti);");
	assert(4, ({ typedef float tf; sizeof(tf); }), "typedef float tf; sizeof(tf);");
	assert(1, ({ typedef _Bool tb; sizeof(tb); }), "typedef _Bool tb; sizeof(tb);");
	/* assert(1, ({ typedef void tv; sizeof(tv); }), "typedef void tb; sizeof(tv);"); */
	assert(8, ({ typedef double td; sizeof(td); }), "typedef double td; sizeof(td);");
	/* assert(0, 0, "0"); */
	/* assert(0, ({ 0; }), "0"); */
	return 0;
}
