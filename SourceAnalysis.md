# 일러두기 #
Moonshell 자체는 미디어 플레이어이면서 Open Source로 공개되어있습니다.
Moonshell 에서 MIDI 파일을 플레이하려면 음원 파일과 해당 플러그인을 같이 복사해서 넣어준담에 기계에 Moonshell에 넣어주면 실제로 MIDI 파일을 불러와서 NDSL로 재생이 가능합니다.
따라서 해당 플러그인을 분석할 필요가 있고, 심지어 그냥 카피해서 요리조리 잘 조리하면 프로젝트를 쉽게 완수할 수 있을것 같습니다..

예상되는 문제점 : 현재 smidlib 부분은 ARM9에서 돌리고 있습니다 아시다시피 NDSL에서는 ARM7과 ARM9 두개로 만들어져있는데,
미디를 ARM9에서 돌려버리면 그래픽 연산 같은것은 어떻게 될지 잘 모르겠습니다. Moonshell 만든 작자가 ARM7으로 옮기려다가 귀차니즘때문에 그만둔듯;

# MoonShell의 플러그인 구성에 관하여 #
  * 진모형이 주석단 부분을 참조로 좀더 자세하게 따라가보았습니다.
  * 소스가 플러그인 형태이기 때문에 상당히 Moonshell 자체에 의존적인데, 따라서 먼저 플러그인의 종류를 좀 봐야합니다.
    1. plugin.h에서 다음과 같은 플러그인 지원을 위해 [공통적인 함수(TPlugin\_StdLib)](http://code.google.com/p/codecomposer/source/browse/trunk/mspv21_midrcp/arm9/source/plugin.h#15) 를 제공하고 있습니다.
    1. 그리고 이미지용 플러그인은 [다음 구조체에 정의된 함수](http://code.google.com/p/codecomposer/source/browse/trunk/mspv21_midrcp/arm9/source/plugin.h#102)를 추가적으로 사용이 가능합니다.
    1. 그리고 우리가 분석하는 함수는 [다음과 같은 구조체에 정의된 함수](http://code.google.com/p/codecomposer/source/browse/trunk/mspv21_midrcp/arm9/source/plugin.h#121)를 정의해서 구현해야 하는것 같습니다. 플러그인에 있는 Main.cpp에 있는 함수가 이러한 내용이 구현되어있습니다.
```
#ifdef PluginMode_Sound

typedef struct {
  bool (*Start)(int FileHandle);
  void (*Free)(void);
  
  u32 (*Update)(s16 *lbuf,s16 *rbuf);
  
  s32 (*GetPosMax)(void);
  s32 (*GetPosOffset)(void);
  void (*SetPosOffset)(s32 ofs);
  
  u32 (*GetChannelCount)(void);
  u32 (*GetSampleRate)(void);
  u32 (*GetSamplePerFrame)(void);
  int (*GetInfoIndexCount)(void);
  
  bool (*GetInfoStrL)(int idx,char *str,int len);
  bool (*GetInfoStrW)(int idx,UnicodeChar *str,int len);
  bool (*GetInfoStrUTF8)(int idx,char *str,int len);
} TPlugin_SoundLib;

#endif
```
    * **위의 함수 중 Start와 Update 부분이 중요한편이며, 실제로 어떻게 함수가 Moonshell에서 호출이 되는지 살펴볼 필요가 있다.**
## Update 함수 분석 ##
    * Update 함수가 어디서 시작되는지 보려면 다음과 같이 main함수에서 mainloop안을 살펴봐야한다.
```
int main(void)
{
  ResetMemory();
  
  REG_IME=0;
  
  POWER_CR = POWER_ALL_2D;
  //....(중략)

  {
    extern void mainloop(void);
    Splash_Update();
    mainloop();
  }
  
  glDefaultClassFree();
  
  ShowLogHalt();
}
```
  * void mainloop(void) 부분을 실제로 보면 화면이 갱신될때마다 해당 pcm 부분을 업데이트하는것 같다.
    * 이 부분까지 다 가지고 가야할것 같다는 생각.
    * 혹은 그런 부분이 있다면 이렇게 Update해주는 부분을 호출하는 식으로 가야할듯.
```
// 이 부분을 보면 리셋 요청이 들어오기 전까지 계속해서 무한반복을 돌고 있다.
while(RequestSystemReset==false){ cwl();

    // ... 중략
    while(strpcmUpdate_mainloop()==true){ cwl();
      if(GlobalINI.KeyRepeat.DelayCount<VsyncPassedCount) break;
    }
    
    while(VsyncPassedCount==0){ cwl();
      strpcmUpdate_mainloop();
      swiWaitForIRQ();
    }
    
    if(IPC3->RequestShotDown==true){
      videoSetModeSub_SetShowLog(true);
      Proc_Shutdown();
      while(1);
    }
    // ... 중략
}
```
  * bool strpcmUpdate\_mainloop(void) 를 실제 보게되면 ldst와 rdst가 뭔지 볼 수 있다.
    * 여기서 Samples=pPluginBody->pSL->Update(ldst,rdst); 하는 식으로 호출을 하는 것을 볼 수 있다.
```
 // ... 중략
  s16 *ldst=&strpcmRingLBuf[BaseSamples*CurIndex];
  s16 *rdst=&strpcmRingRBuf[BaseSamples*CurIndex];
  
  if(strpcmRequestStop==true){ cwl();
    Samples=0;
    }else{ cwl();
    
    switch(ExecMode){ cwl();
      case EM_MSPSound: case EM_MP3Boot: { cwl();
        if(strpcmDoubleSpeedFlag==true){ cwl();
          pPluginBody->pSL->Update(NULL,NULL);
        }
        Samples=pPluginBody->pSL->Update(ldst,rdst);
 // ... 중략
```
## Start 함수 분석 ##
  * 여기서 ` bool (*Start)(int FileHandle); `부분은 다음과 같은 곳에서 호출을 하고 있습니다.
    * ` pPluginBody->pSL->Start(FileHandle) ` 라는 부분으로 MoonShell 전체에서 찾아보면 Main\_fs.h 파일에 다음과 같이 사용을 하고 있습니다.
    * 정확히 분석할 필요는 없으나, 다음과 같은 식으로 사용이 됩니다.
      * 대충 보면 플러그인을 로딩하고, 해당 파일을 열어서 플러그인쪽으로 던져주는 식이네요.
      * if(pPluginBody->pSL->Start(FileHandle)==false){ cwl(); ... 이런식입니다.
  * 그래도 이 함수 부분을 보면 FileHandle을 선언하고 Shell\_OpenFile 같은 함수를 이용하여 파일을 읽어들이는 부분을 볼 수 있다.

```
  if(ExecMode==EM_MSPSound){ cwl();
    {
      char fn[PluginFilenameMax];
      char ext[ExtMaxLength];
      
      FileSys_GetFileExt(FileIndex,ext);
      DLLList_GetPluginFilename(ext,fn);
      pPluginBody=DLLList_LoadPlugin(fn);
    }
    MWin_AllocMem(WM_PlayControl);
    if(FS_FileOpen(FileIndex)==false) return;

    if(pPluginBody->pSL->Start(FileHandle)==false){ cwl();
      FS_ExecuteFile_LoadFail();
      PlugInfo_ShowCustomMessage_LoadError();
      FS_ExecuteFile_InitFileInfoWindow();
      return;
    }
    FS_ExecuteFile_InitFileInfoWindow();
    FS_ExecuteFile_InitPlayControlWindow(FileIndex);
    
    u32 SampleRate=pPluginBody->pSL->GetSampleRate();
    u32 SamplePerFrame=pPluginBody->pSL->GetSamplePerFrame();
    u32 ChannelCount=pPluginBody->pSL->GetChannelCount();
    EstrpcmFormat SPF=FS_ExecuteFile_GetOversamplingFactorFromSampleRate(SampleRate);
    
    for(u32 idx=0;idx<SoundEffectPluginCount;idx++){
      pSoundEffectPluginBody[idx]=DLLList_LoadSoundEffectPlugin(idx);
      if(pSoundEffectPluginBody[idx]!=NULL){
        if(pSoundEffectPluginBody[idx]->pSE->Start(SampleRate,SamplePerFrame,ChannelCount)==false){
          DLLList_FreeSoundEffectPlugin(idx);
          safefree(pSoundEffectPluginBody[idx]); pSoundEffectPluginBody[idx]=NULL;
        }
      }
    }
    
    Resume_Backup(true);
    strpcmStart(false,SampleRate,SamplePerFrame,ChannelCount,SPF);
    PrintFreeMem();
    return;
  }
```
## MIDI 파트의 대략적인 구성 ##
  * 먼저 분석하기 전에 이 그림을 참조합시다. 정확하지는 않습니다만, 대략 분석해보니 이런 구조임.
![http://farm4.static.flickr.com/3122/2554900175_3d8529bb9c_o.png](http://farm4.static.flickr.com/3122/2554900175_3d8529bb9c_o.png)

  * 여기서 가장 하단의 Smidlib\_pch.**부분은 왜 그렇게 짰는지에 대한 레퍼런스도 없을뿐더러 소스의 양이 상당하므로 사용에 중점을 둬야할것 같습니다.**


## 아직 정리를 못한 부분 ##

// 일단 해당 소스를 보면...

bool Start(int FileHandle)
{
> // 시작부터 쌩뚱맞게 INI이 뭐지? 라고 생각이 들텐데...
> // 같은 디렉토리에 MidRcp.ini 파일이 있는데 이 부분에 뭔가 설정해주는 것 같음. 실제로 GetINIData가 뭔지 추적을 할수가 없긴한데...
> // 실제로 아래와 같이 INI 파일을 불러온 다음에 StackCount=GlobalINI.MIDPlugin.DelayStackSize; 이런식으로 직접 접근을 해서 쓰더라능

> // Plugin\_def.h 에 #define GetINIData (pStdLib->GetINIData) 로 정의되어 있으며, const TPlugin\_StdLib **pStdLib; 로 선언되어 있는 것도 알 수 있다.**

> // pStdLib 포인터는 Plugin\_dll.cpp 에 정의된 LoadLibrary함수에 의해 값이 할당되는데, 이 LoadLibrary함수를 콜하는 부분이 아무리 봐도 여기엔 존재하지 않는다-_-;_

> // TPlugin\_StdLib 구조체는 Plugin.h 파일에 정의되어 있으며,상당히 많은 변수들을 포함하고 있어서 링크는 쉽지가 않음
> InitINI();
> LoadINI(GetINIData(),GetINISize());

> // 뭔가 스택 초기화 부분 같은데, 정확히 소스만 봐서는 뭔말인지 파악하기 힘들다. 일단 미디에 초첨을 맞춰서 전체적인 흐름을 보는 것으로 합시다.
> if(Start\_InitStackBuf()==false) return(false);

> TiniMIDPlugin **MIDPlugin=&GlobalINI.MIDPlugin;**

> {
> > if(MaxSampleRate

&lt;MIDPlugin-&gt;

SampleRate) MIDPlugin->SampleRate=MaxSampleRate;


> SamplePerFrame=MIDPlugin->SampleRate/MIDPlugin->FramePerSecond;
> if(MaxSamplePerFrame<SamplePerFrame) SamplePerFrame=MaxSamplePerFrame;
> SamplePerFrame&=~15;
> if(SamplePerFrame<16) SamplePerFrame=16;

......우왕 뭔말인지 모르겠어요;


> - 진모
이후 setStart까지의 코드를 보면.. FileHandle에 해당하는만큼의 메모리를 할당하는 것을 볼 수 있다.

> DeflateBuf : 해당 파일을 읽어들인 버퍼

> Deflatesize : 해당 파일의 크기

이후 오류검사를 한 뒤, DeflateBuf 를 이용하여 파일 포맷을 검사한다.



Offset


Length

(byte)


Type


Description


Value

00


4


Char[4](4.md)


Chunk ID


“MThd” (0x4D546864)

04


4


Dword


Chunk Size


6 (0x00000006)

08


2


Word


Format Type


0~2

10


2


Word


Number of Tracks


1~65,535

12


2


Word


Time Division





if(strncmp((char_)DeflateBuf,"MThd",4)==0){
> **consolePrintf("Start Standard MIDI file.\n");
> detect=true;
> FileFormat=EFF\_MID;
}
if(strncmp((char**)DeflateBuf,"RCM-PC98V2.0(C)COME ON MUSIC",28)==0){
>_consolePrintf("Start RecomposerV2.0 file.\n");
> detect=true;
> FileFormat=EFF\_RCP;
}



우리가 세미나에서 알아봤듯이 첫 4바이트의 값이 MThd 인지 확인을 해서 미디 파일인지 구분하는 것을 알 수 있다.

MID or RCP 둘 다 아니면 false를 리턴하며 종료하게 된다.



이후엔, 클락 세팅을 한다.


2> selStart ( Main.cpp )
// 일단 소스의 시작은 미디 파일의 로딩부터 시작을 해보려고 합니다.
static bool selStart(void)
{
> switch(FileFormat){
> > case EFF\_MID: return(smidlibStart()); break;
> > case EFF\_RCP: return(rcplibStart()); break;

> }

> return(false);
}

이라는 부분부터 출발! 여기서 보면 이 모듈은 MID와 RCP라는 두개의 음원 종류를 재생하기 때문에 파일 포맷에 따라 초기화해주는 부분이 차이가 나는 것을 확인 가능.

2> smidlibStart (smidlib.cpp ) - #1
1>에 이어서 smidlibStart 부분을 찾아가면........다음과 같은 진정한 초기화 루틴이 나옵니다.
먼저 SM\_Init으로 메모리 초기화와 구조체 초기화를 때려줍니다.
bool smidlibStart(void)
{
> TStdMIDI _**StdMIDI=&StdMIDI;**

> SM\_Init();                                          // 위에서 정의한 전역변수인 StdMIDI를 사용전에 메모리 초기화해주고 각 구조체의 멤버를 0으로 지정합니다.
> > // 함수에 대해서 특별한 내용은 없어서 코드는 생략합니다.

> SM\_LoadStdMIDI(bdata,bSampleRate);   //  이 부분은 아래 참조._

// ......일단 이런 과정이 있으니 실제 호출 부분으로 점프해봅시다

3> SM\_LoadStdMIDI
// 우왕 실제로 파일 불러오나봅니다.
void SM\_LoadStdMIDI(u8 **FilePtr,u32 SampleRate)
{
> TStdMIDI**_StdMIDI=&StdMIDI;_

> _StdMIDI->File=FilePtr;
>_StdMIDI->FilePos=0;

> _StdMIDI->SampleRate=SampleRate;
>_StdMIDI->SamplePerClockFix16=0;

> SM\_LoadChank();                        // 하단의 해당함수 분석 참조.
> SM\_ProcMetaEvent\_InitTempo();    // 이부분은 뭐하는건지 잘 모르겠음; somebody explain to us!

> // 디버깅용으로 화면에 찍어보는 부분인듯.
> {
> > TSM\_Chank **_SM\_Chank=&StdMIDI.SM\_Chank;
> >_consolePrintf("Chank:Len=%d frm=%d trk=%d res=%d\n",_SM\_Chank->Len,_SM\_Chank->Format,_SM\_Chank->Track,_SM\_Chank->TimeRes);

> }**

> // 스펙상으로 인하면 트랙의 개수가 6만 몇개인데, 32개로 제한을 해둔 것 같다. 왜 그럴까? somebody explain to us!
> if(SM\_TracksCountMax

<\_StdMIDI->

SM\_Chank.Track) _StdMIDI->SM\_Chank.Track=SM\_TracksCountMax;_

> // 우왕 실제로 트랙 불러옵니다! 그전에 메모리 초기화부터 해주는 센스.
> MemSet32CPU(0,&_StdMIDI->SM\_Tracks,SM\_TracksCountMax\*sizeof(TSM\_Track));_

> // 실제로 하단의 루프는 트랙을 실제로 메모리로 불러옵니다. 일단 루프를 돌기 위해서 변수 초기화.
> u32 TrackCount=_StdMIDI->SM\_Chank.Track;_

> // 루프를 돌면서 각 트랙의 데이터를 SM\_Tracks 배열 안으로 데이터를 긁어옵니다. 이 과정에서 SM\_LoadTrackChank 함수를 호출해서 진행하는데...하단에 자세한 설명!
> for(u32 TrackNum=0;TrackNum<TrackCount;TrackNum++){
> > TSM\_Track **pSM\_Track=&_StdMIDI->SM\_Tracks[TrackNum](TrackNum.md);
> > SM\_LoadTrackChank(pSM\_Track);
> >_consolePrintf("Track.%d:length=%d\n",TrackNum,(u32)pSM\_Track->DataEnd-(u32)pSM\_Track->Data);

> }
}**

4> SM\_LoadChank
// 우왕 이부분이 드디어 MIDI파일의 HeaderChunk를 읽어오는 부분입니다.
// 스펙을 잠깐 보시려면 이 페이지를 참조 해서 Header Chunk로 검색해서 보세요. 징짜 스펙대로 만든 케이스.
// 스펙에 의하면 MIDI의 데이터 단위는 청크이므로 여기서도 청크 구조체를 선언하고 그런식으로 헤더만 불러온다는거.
static void SM\_LoadChank(void)
{
> TSM\_Chank _**SM\_Chank=&StdMIDI.SM\_Chank;**

>_SM\_Chank->ID[0](0.md)=SM\_ReadByte();
> _SM\_Chank->ID[1](1.md)=SM\_ReadByte();
>_SM\_Chank->ID[2](2.md)=SM\_ReadByte();
> _SM\_Chank->ID[3](3.md)=SM\_ReadByte();_

> _SM\_Chank->Len=SM\_ReadBigDWord();
>_SM\_Chank->Format=SM\_ReadBigWord();
> _SM\_Chank->Track=SM\_ReadBigWord();
>_SM\_Chank->TimeRes=SM\_ReadBigWord();

> if(_SM\_Chank->Format==0)_SM\_Chank->Track=1;
}

5> SM\_LoadTrackChunk
// 헤더를 불러운 다음에 트랙 데이터를 _StdMIDI->SM\_Tracks[TrackNum](TrackNum.md) 안으로 불러오는 과정인데
// 여기서 ReadSkip 하면 len만큼 파일 포인터를 뒤로 밀어버리는 과정이다.
// 위의 과정과 같이  이 페이지를 참조 하면 Track Chunk 부분의 스펙대로 제작한 케이스임을 알 수 있다.
// 현재 SM\_ReadByte이니 SM\_ReadBigDWord 같은 과정들도 데이터를 읽어오면서 파일포인터를 그만큼 뒤로 미룬다.
// 특별히 트랙의 데이터에 대해서는 터치를 하지 않고 있다.
// 대충 이부분까지 끝나면 다시 2>로 돌아가자! (파일 로딩이 끝난것임)_

static void SM\_LoadTrackChank(TSM\_Track **pSM\_Track)
{
> pSM\_Track->EndFlag=false;**

> pSM\_Track->ID[0](0.md)=SM\_ReadByte();
> pSM\_Track->ID[1](1.md)=SM\_ReadByte();
> pSM\_Track->ID[2](2.md)=SM\_ReadByte();
> pSM\_Track->ID[3](3.md)=SM\_ReadByte();

> u32 len=SM\_ReadBigDWord();

> pSM\_Track->Data=&StdMIDI.File[StdMIDI.FilePos];
> pSM\_Track->DataEnd=&pSM\_Track->Data[len](len.md);

> SM\_ReadSkip(len);

> pSM\_Track->RunningStatus=0;
> pSM\_Track->WaitClock=0;
}

6> smidlibStart (smidlib.cpp ) - #2
> /// ....2>에 이어서
> // 우왕 SMF Type2는 지원을 아예 안하는군요 ㅋ;
> > if(_StdMIDI->SM\_Chank.Format==2){
> > >_consolePrintf("not support Format2\n");
> > > return(false);

> > }


> // 다음 함수는 실제 사운드 부분을 초기화 하는 부분 같음 : 우왕 핻당 소스를 봤는데 무슨 말인지 전혀 모르겠군요; 일단 스킵;
> > PCH\_Init(bSampleRate,bSampleBufCount,bMaxChannelCount,bGenVolume);

> // MTRKCC\_Init은 뭐하는걸까요?  <-- MTRK 는 Midi Track 의 약자로 알고 있는데 실제 정의부분을 보면 Track을 초기화하고 있는듯 - 진모
> > MTRKCC\_Init();


> u32 TrackCount=_StdMIDI->SM\_Chank.Track;_

> for(u32 TrackNum=0;TrackNum<TrackCount;TrackNum++){
> > TSM\_Track _pSM\_Track=&**StdMIDI->SM\_Tracks[TrackNum](TrackNum.md);
> > pSM\_Track->WaitClock=SM\_GetDeltaTime(pSM\_Track);

> }**

> return(true);
}_

7> MTRKCC\_Init


typedef struct {
> TMTRK\_Track Track[MTRK\_TrackCount](MTRK_TrackCount.md);
} TMTRK;



static ALIGNED\_VAR\_IN\_DTCM TMTRK MTRK;



static ALIGNED\_VAR\_IN\_DTCM TMTRK\_Track DrumMap1,DrumMap2;



void MTRKCC\_Init(void)
{
> MemSet32CPU(0,&MTRK,sizeof(TMTRK));            // 미디파일의 모든 트랙을 0으로 초기화, TMTRK <-- 모든 MTRK를 갖고 있는 구조체
> for(u32 trk=0;trk<MTRK\_TrackCount;trk++){       // 각각의 트랙을 초기화
> > MTRKCC\_InitTrack(&MTRK.Track[trk](trk.md),trk);

> }

> MemSet32CPU(0,&DrumMap1.Program,sizeof(TMTRK\_Track));
> MTRKCC\_InitTrack(&DrumMap1,0xfe);
> MemSet32CPU(0,&DrumMap2.Program,sizeof(TMTRK\_Track));
> MTRKCC\_InitTrack(&DrumMap2,0xff);
}