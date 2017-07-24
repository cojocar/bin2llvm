int call_me(int, char **);

int
main(int argc, char *argv[])
{
	call_me(argc-1, argv);
	switch (argc) {
	case 0: call_me(argc-1, argv); break;
	case 1: call_me(argc+1, argv); break;
	case 2: call_me(argc-2, argv); break;
	case 3: call_me(argc-5, argv); break;
	case 4: call_me(argc+3, argv); break;
	case 5: call_me(argc+2, argv); break;
	default:
			break;
	}
	return 0;
}

int
call_me(int x, char *p[])
{
	char *a = p[x];
	char *orig_a = a;
	while (*a)
		++a;
	return (int)a-(int)orig_a;
}
