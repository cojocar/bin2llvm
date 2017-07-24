int call_me(char *a);
int more_calls(char x);

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
	if (more_calls(*orig_a))
		return (int)a-(int)orig_a;
	else
		return more_calls(*(orig_a+1));
}

int
more_calls(char x)
{
	if (x > 'A') {
		return 'a'-x;
	} else {
		return 'A'-x;
	}
}
