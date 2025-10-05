#define LINE_LEN		      128
#define CMD                     0   /*flag for cmd_gets() input cmd*/
#define PWD                     1   /*flag for cmd_gets() input passwd*/
#define USERNAME_PASSWD_LEN	   16   /*length of username or passwd*/
#define NULL    0

static char *cmd_gets(char *buf, int len, int flag)
{
#define KEY_BS			0x08
#define KEY_CR			0x0D

	int c,i=0;
	char *cp;

	cp = buf;
	while ((c = getc()) != KEY_CR) 
	{
		if ( c == KEY_BS ) 
		{
			if ( cp != buf ) 
			{
				printf("\b \b");
				cp--;
				i--;
			}
		} 
		else
		{
			if ( buf != NULL ) 
			{
				if ( i <= len-1 )			
				{
					if ( flag == 0x0 )
						putc(c); /*cmd mode*/
					else 
						putc('*'); /*passord mode*/
					*cp++ = c;
					i++;
				}
			}
		}
	}
	if (buf != NULL)
		*cp = '\0';
	return buf;
}

static int trim(char *buf)
{
	int i,j;
	int flag_i=1, flag_j=1;
	for (i=0,j=strlen(buf)-1; i<=j; i++,j--)
	{
		if (flag_i)
		{	
			if((buf[i] != ' ') && (buf[i] != '\t'))
			flag_i = 0;
		}
		if (flag_j)
		{
			if((buf[j] == ' ') || (buf[j] == '\t'))
				buf[j] = 0;
			else
				flag_j = 0;
		}
		if ((flag_i == 0) && (flag_j == 0))
			break;
	}
	return i;
}

int ecnt_abortboot_keyed(int bootdelay)
{
	unsigned char UserName[LINE_LEN];
    unsigned char Pwd[LINE_LEN];
	int abort = 0;
	int i;

#ifdef TCSUPPORT_AUTOBENCH
	/* always abort login username/passwd */
	return 1;
#endif

    do {
#if 0
        memset(UserName, 0, LINE_LEN);
        memset(Pwd, 0, LINE_LEN);
        printf("UserName: ");
        cmd_gets(UserName, LINE_LEN, CMD);
        printf("\n");
        printf("Password: ");
        cmd_gets(Pwd, LINE_LEN, PWD);
        i = trim(UserName);
        printf("\n\n");
        if(( !strncmp(&UserName[i], getenv ("username"), USERNAME_PASSWD_LEN))
			&& (!strncmp(Pwd, getenv ("password"), USERNAME_PASSWD_LEN)))
#else
	if(1)
#endif
        {
	        abort = 1;
	        break;
        }
    } while (!abort);	

	return abort;
}
