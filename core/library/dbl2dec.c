#include "string.h"
#include "math.h"

int dbl2dec(double val, char* buff, int width)
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

