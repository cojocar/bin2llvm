extern void call_me_from_thumb(void);

void
call_me_from_arm(void)
{
	call_me_from_thumb();
}
