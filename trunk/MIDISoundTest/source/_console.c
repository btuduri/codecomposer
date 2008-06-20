#include <nds.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "_console.h"

static s32 _udeci(u32 u,s8 *s,s32 i)
{
	u32 div,flg;
	div=1000000000;
	flg=0;
	for(;;){
		if(u/div){
			s[i++]='0'+(u/div);
			flg=1;
			u-=(u/div)*div;
		}
		else if(flg || (div==1)){
			s[i++]='0';
		}
		div=div/10;
		if(div==0) break;
	}
	return(i);
}

const static s8 hexchar[16]={
'0','1','2','3','4','5','6','7',
'8','9','A','B','C','D','E','F'
};

static s32 _hex(u32 u,s8 *s,s32 i,s32 keta){
	u32 div,flg,k;
	div=28;
	flg=0;
	for(k=8;k>0;k--){
		if(u>>div){
			s[i++]=hexchar[u>>div];
			flg=1;
			u-=(u>>div)<<div;
		}
		else if(flg || (div==0) || (keta>=k)){
			s[i++]='0';
		}
		div=div-4;
	}
	return(i);
}

void _consolePrint(char const* pstr)
{
  // PcPrint(pstr);
}

void _consolePrintf(char const* format, ...)
{
	va_list args;
	s32 i,d,keta;
	u32 u;
	s8 *ss;
	s8 *s;
	s8 buf[255];
	char chr;

	s=buf;
	va_start(args,format);
	
	for(i=0;;){
		chr = *format++;
		if(chr=='%'){
			keta=0;
again:
			chr = *format++;
			switch(chr){
			case 'd':		//	%d:ïÑçÜÇ†ÇËÇPÇOêiêî
				d=va_arg(args,s32);
				if(d<0){
					s[i++]='-';
					u=-d;
				}
				else{
					u=d;
				}
				i=_udeci(u,s,i);
				break;
			case 'u':		//	%u:ïÑçÜÇ»ÇµÇPÇOêiêî
				u=va_arg(args,u32);
				i=_udeci(u,s,i);
				break;
			case 'X':
			case 'x':		//	%x:ÇPÇUêiêî
				u=va_arg(args,u32);
				i=_hex(u,s,i,keta);
				break;
			case 's':		//	%s:ï∂éöóÒ
				ss=va_arg(args,s8 *);
				for(d=0;;d++){
					if(ss[d]==0) break;
					s[i++]=ss[d];
				}
				break;
			case 'c':		//	%c:ï∂éö
				d=va_arg(args,s32);
				s[i++]=d;
				break;
			default:
				if((chr>='0')&&(chr<='9'))
				{
					keta = keta * 10 + chr-'0';
					goto again;
				}
				// direct
				s[i++]=chr;
			}
		}
		else{
			if((i>240)||(chr==0)){
				s[i++]=0;
				break;
			}
			s[i++]=chr;
		}
	}
	va_end(args);
	
    _consolePrint((char*)buf);
}