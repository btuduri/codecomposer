
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ttaenc.c"

int ttacenc_static_ExecuteEncode(int *pSourceSamples,int SourceSamplesCount,int BitsPerSample,unsigned char *pCodeBuf)
{
  return(compress(pSourceSamples,SourceSamplesCount,BitsPerSample,pCodeBuf));
}

