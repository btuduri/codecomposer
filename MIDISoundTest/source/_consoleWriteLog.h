
#ifndef _consoleWriteLog_h
#define _consoleWriteLog_h

#define cwl(); _cwl(__FILE__,__LINE__,__current_sp());

extern "C" {
void _cwl(char *file,int line,u32 sp);
}

#ifdef __cplusplus
extern "C" {
#endif

void PrfStart(void);
u32 PrfEnd(int data);

#ifdef __cplusplus
}
#endif

#endif
