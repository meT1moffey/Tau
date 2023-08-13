func main {
	vars ptr a;
	a 80 create =;

	a 0 * 1 =;
	a 8 + 0 * 1 =;

	vars long i;
	i 0 =;

	mark begin
	i 16 + 80 == ? jump end;
		a i 16 + + 0 * a i + 0 * a i 8 + + 0 * + =;
		i i 8 + =;
	jump begin;
	mark end;
	
	i 0 =;
	vars long n;
	mark begin1
	i 80 == ? jump end1;
		n a i + 0 * =;
		i i 8 + =;
		log;
	jump begin1;
	mark end1;

	a destroy;
}