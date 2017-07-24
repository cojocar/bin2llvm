int call_me(char *a);

int
main(void)
{
	call_me(0);
	return 0;
}

int
call_me(char *a)
{
	char *orig_a = a;
	while (*a)
		++a;
	return (int)a-(int)orig_a;
}
