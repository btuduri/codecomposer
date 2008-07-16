
#ifndef MediaType_h
#define MediaType_h

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {DIMT_NONE=0x454e4f4e,
              DIMT_MPCF=0x4643504d,
              DIMT_M3CF=0x4643334d,DIMT_M3SD=0x4453334d,
              DIMT_SCCF=0x46434353,DIMT_SCSD=0x44534353,
              DIMT_EZSD=0x44535a45,
              } EDIMediaType;
//,DIMT_MPCF,DIMT_MPSD,DIMT_M3CF,DIMT_M3SD,DIMT_SCCF,DIMT_SCSD,DIMT_FCSR,DIMT_EZSD,DIMT_MMCF,DIMT_SCMS,DIMT_EWSD,DIMT_NMMC,DIMT_NJSD,DIMT_DLMS,DIMT_G6FC,DIMT_R4TF,DIMT_EZ5S} EDIMediaType;

extern EDIMediaType DIMediaType;
extern const char *DIMediaName;
extern char DIMediaID[5];

#ifdef __cplusplus
}
#endif

#endif
