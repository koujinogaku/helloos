#include "stdarg.h"
#include "print.h"
#include "display.h"
#include "math.h"
#include "string.h"

/*
specifier	contents	Use case
%d	Decimal	%d %04d %6d
%x	Hexadecimal(Lower case)	%x %04x %6x
%X	Hexadecimal(Capital)	%X %04X %6X
%c	charactor	%c
%s	String	%s
%%	%(Percent)Specifies the character itself	%%
*/


static int tsprintf_string(char* str,char* buff);
static int tsprintf_char(int ch,char* buff);
static int tsprintf_decimal(signed long val,char* buff,int zf,int wd);
static int tsprintf_hexadecimal(unsigned long val,char* buff, int capital,int zf,int wd);
static int tsprintf_double(double val, char* buff, int width);

int print_sformat(char* buff,const char* fmt, ...)
{
  va_list arg;
  int len;

  va_start(arg, fmt);

  len = print_vsformat(buff,fmt,arg);

  va_end(arg);

  return len;
}

int print_format(const char* fmt, ...)
{
  char buff[1024];
  va_list arg;
  int len;

  va_start(arg, fmt);

  len = print_vsformat(buff,fmt,arg);

  va_end(arg);

  display_puts(buff);
  return len;
}

int print_vsformat(char* buff,const char* fmt,va_list arg)
{
    int len;
    int size;
    int zeroflag,width;
    int val;

    len = 0;

    while(*fmt){
        size = 0;
        if(*fmt=='%'){        /* Process related to "%"  */
            zeroflag = width = 0;
            fmt++;

            if (*fmt == '0'){
                fmt++;
                zeroflag = 1;
            }
            if ((*fmt >= '0') && (*fmt <= '9')){
                width = *(fmt++) - '0';
            }

            /* printf ("zerof = %d,width = %d\n",zeroflag,width); */

            switch(*fmt){
            case 'd':        /* Decimal */
		val = va_arg(arg,int);
                size = tsprintf_decimal(val,buff,zeroflag,width);
                break;
            case 'x':        /* Hexadecimal 0-f */
                size = tsprintf_hexadecimal(va_arg(arg,int),buff,0,zeroflag,width);
                break;
            case 'X':        /* Hexadecimal 0-F */
                size = tsprintf_hexadecimal(va_arg(arg,int),buff,1,zeroflag,width);
                break;
            case 'c':        /* character */
                size = tsprintf_char(va_arg(arg,int),buff);
                break;
            case 's':        /* ASCIIZ String */
		val = (int)va_arg(arg,char*);
                size = tsprintf_string((char*)val,buff);
                break;
            case 'f':
		val = va_arg(arg,double);
                size = tsprintf_double(val, buff, width);
		break;
            default:        /* other than control code */
                /* %%(% charactor) is performed */
                len++;
                *(buff++) = *fmt;
                break;
            }
            len += size;
            buff += size;
            fmt++;
        } else {
            *(buff++) = *(fmt++);
            len++;
        }
    }

    *buff = '\0';        /* termination */

    return len;
}
/*
  translation from value to decimal string
*/
static int tsprintf_decimal(signed long val,char* buff,int zf,int wd)
{
	int i;
	char tmp[16];
	char* ptmp = &tmp[15];
	int len = 0;
	int minus = 0;

	if (!val){		/* value is 0 */
		*ptmp = '0';
		ptmp--;
		len++;
	} else {
		/* if value is minus then make complement on 2 */
		if (val < 0){
			val = ~val;
			val++;
			minus = 1;
		}
		while (val){
			/* for buffer under flow */
			if (len >= 8){
				break;
			}
	
			*ptmp = (val % 10) + '0';
			val /= 10;
			ptmp--;
			len++;
		}

	}

	/*  process related to sign and digit alignment */
	if (zf){
		if (minus){
			wd--;
		}
		while (len < wd){
			*(ptmp--) =  '0';
			len++;
		}
		if (minus){
			*(ptmp--) = '-';
			len++;
		}
	} else {
		if (minus){
			*(ptmp--) = '-';
			len++;
		}
		while (len < wd){
			*(ptmp--) =  ' ';
			len++;
		}
	}

	/* copy the generated string */
	for (i=0;i<len;i++){
		*(buff++) = *(++ptmp);
	}

	return len;
}

/*
  translate value to Hexadecimal
*/
static int tsprintf_hexadecimal(unsigned long val,char* buff, int capital,int zf,int wd)
{
	int i;
	char tmp[16];
	char* ptmp = &tmp[15];
	int len = 0;
	char str_a;

	/* switch translation mode A-F to capital or low-case */
	if (capital){
		str_a = 'A';
	} else {
		str_a = 'a';
	}
	
	if (!val){		/* if value is zero */
		*(ptmp--) = '0';
		len++;
	} else {
		while (val){
			/* Measures buffer under-flow */
			if (len >= 8){
				break;
			}

			*ptmp = (val % 16);
			if (*ptmp > 9){
				*ptmp += str_a - 10;
			} else {
				*ptmp += '0';
			}
		
			val >>= 4;		/* Divided by 16 */
			ptmp--;
			len++;
		}
	}
	while (len < wd){
		*(ptmp--) =  zf ? '0' : ' ';
		len++;
	}
		
	for (i=0;i<len;i++){
		*(buff++) = *(++ptmp);
	}

	return len;
}

/*
  translate value to 1-charactor
*/
static int tsprintf_char(int ch,char* buff)
{
	*buff = (char)ch;
	return 1;
}

/*
  translate value to ASCIIZ
*/
static int tsprintf_string(char* str,char* buff)
{
	int count = 0;
	while(*str){
		*(buff++) = *str;
		str++;
		count++;
	}
	return count;
}

/*
  translate value to float point decimal string
*/


static int tsprintf_double(double val, char* buff, int width)
{
	int minus=0,is_exp=0;
	int exp_digit,dot_pos;
	int digit;
	double top_base,exp_f;
	int i;
	int count=0;

	if(width==0)
		width=9;

	if(val<0) {
		minus=1;
		val = fabs(val);
	}

	exp_f = log10(val);
	if(exp_f >= 0.0) {
		dot_pos = exp_digit = (int)exp_f;
	}
	else {
		dot_pos = exp_digit = ((int)exp_f)-1;
	}

	if( exp_digit >= 0 ) {
		if( exp_digit >= width ) {
			is_exp = 1;
			dot_pos = 0;
		}
	}
	else {
		if(-exp_digit <= (width / 2)) {
			exp_digit = -1;
			dot_pos = -1;
		}
		else {
			is_exp = 1;
			dot_pos = 0;
		}
	}
	top_base = bpow(10.0,exp_digit);

	if(minus) {
		*(buff++) = '-';
		count++;
	}
	dot_pos++;
	for(i=0; i<width; i++) {
		if(dot_pos==0) {
			*(buff++) = '.';
			count++;
		}
		digit = (int)(val / top_base);
		*(buff++) = '0'+digit;
		count++;
		val = val - (digit*top_base);
		top_base = top_base / 10;
		dot_pos--;
	}
	if(is_exp) {
		*(buff++) = 'e';
		count++;
		if(exp_digit>0) {
			*(buff++) = '+';
			count++;
		}
		sint2dec(exp_digit,buff);
		{
			int len;
			len = strlen(buff);
			buff += len;
			count += len;
		}
	}

	return count;
}

