extern void call_me_from_arm(void);
int
main(void)
{
	call_me_from_arm();
	return 0;
}

void
call_me_from_thumb(void)
{
}
