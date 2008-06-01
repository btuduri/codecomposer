unit MainWin;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls,math,zlib;

type
  TForm1 = class(TForm)
    Memo1: TMemo;
    ListBox1: TListBox;
    ListBox2: TListBox;
    procedure FormCreate(Sender: TObject);
  private
    { Private êÈåæ }
  public
    { Public êÈåæ }
    procedure CreateBIN(binfn,prefn:string;_PCM8Bit,_PCMHalfRate:boolean);
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

const pcmfn:string='current.pcm';

const PAT_HeaderSize=239;
const PAT_PCMHeaderSize=7+1+4+4+4+2+4+4+4+2+1+3+3+3+3+1+1+1+1+1+1+1+2+2+36;

const cTrue=1;
const cFalse=0;

var
  GusPath:string;

// convert attributes
var
  PCM8Bit:boolean;
  PCMHalfRate:boolean;

// ------------------

type
  TPAT_Header=record
    ID:array[0..22] of char; // ofs 0
    Discription:array[0..60] of char; // ofs 22
    Channels:byte; // ofs 84
    Waveforms:word; // ofs 85
    MasterVolume:word; // ofs 87
    DataSize:dword; // ofs 89
    InstrumentID:word; // ofs 129
    InstrumentName:array[0..16] of char; // ofs 131
    InstrumentSize:dword; // ofs 147
    Layers:byte; // ofs 151
    LayerDuplicate:byte; // ofs 192
    Layer:byte; // ofs 193
    LayerSize:dword; // ofs 194
    NumberOfSamples:byte; // ofs 198
  end;

var
  PAT_Header:TPAT_Header;

type
  TPAT_PCM=record
    Offset:integer;
    ID:array[0..7] of char;
    Size:integer;
    Fractions:byte;
    LoopStart:dword;
    LoopEnd:dword;
    SampleRate:word;
    RootFrequency:dword;
    VibSweep,VibRatio:double;
    VibDepth:dword;
    EnvRate,EnvOfs:array[0..6-1] of dword;
    sm16bit,smSigned,smLoop,smPing,smEnvelope:boolean;
    Data:array of byte;
  end;

var
  PAT_PCM:array of TPAT_PCM;

type
  TProgram=record
    patfn:string;

    SampleRate:dword;
    RootFreq:dword; // fix16.16
    MasterVolume:dword;
    FractionStart,FractionEnd:dword;
    Length,LoopStart,LoopEnd:dword;
    LoopFlag,s16Flag,EnvelopeFlag:dword;
    Amp:dword;
    Note,Pan:integer;
    VibSweep,VibRatio,VibDepth:dword;
    EnvRate,EnvOfs:array[0..6-1] of dword;
    Data:array of byte;
    DataCount:integer;
  end;

var
  ProgramMap:array[0..127,0..16] of TProgram;
  ProgramMapPatCount:array[0..127] of integer;

procedure InitProgramMap;
var
  prgidx:integer;
begin
  for prgidx:=0 to 128-1 do begin
    ProgramMapPatCount[prgidx]:=0;
  end;
end;

procedure LoadPATHeader(var rfs:TFileStream);
begin
  FillMemory(addr(PAT_Header),sizeof(TPAT_Header),0);

  with PAT_Header do begin
    rfs.Position:=0;
    rfs.ReadBuffer(ID,22);
    rfs.Position:=22;
    rfs.ReadBuffer(Discription,60);
    rfs.Position:=84;
    rfs.ReadBuffer(Channels,1);
    rfs.Position:=85;
    rfs.ReadBuffer(Waveforms,2);
    rfs.Position:=87;
    rfs.ReadBuffer(MasterVolume,2);
    rfs.Position:=89;
    rfs.ReadBuffer(DataSize,4);
    rfs.Position:=129;
    rfs.ReadBuffer(InstrumentID,2);
    rfs.Position:=131;
    rfs.ReadBuffer(InstrumentName,16);
    rfs.Position:=147;
    rfs.ReadBuffer(InstrumentSize,4);
    rfs.Position:=151;
    rfs.ReadBuffer(Layers,1);
    rfs.Position:=192;
    rfs.ReadBuffer(LayerDuplicate,1);
    rfs.Position:=193;
    rfs.ReadBuffer(Layer,1);
    rfs.Position:=194;
    rfs.ReadBuffer(LayerSize,4);
    rfs.Position:=198;
    rfs.ReadBuffer(NumberOfSamples,1);
  end;

  with PAT_Header do begin
    form1.Memo1.Lines.Add(format('Discription:%s',[Discription]));
    form1.Memo1.Lines.Add(format('Instrument(%x:%x):%s',[InstrumentID,InstrumentSize,InstrumentName]));
    form1.Memo1.Lines.Add(format('chs:%d wf:%d Layers:%d,%d,%d,%x',[Channels,Waveforms,Layers,LayerDuplicate,Layer,LayerSize]));
    form1.Memo1.Lines.Add(format('MasterVolume:%d NumberOfSamples:%d',[MasterVolume,NumberOfSamples]));
    form1.Memo1.Lines.Add('');
  end;


end;

procedure LoadPATPCM(var rfs:TFileStream;Index:integer);
var
  str:string;
  SamplingModes:byte;
  idx:integer;
  tmpb:byte;
  tmpdw:dword;
  tmpi:integer;
  erstr,eostr:string;
  sec:double;
  ReadOffset:integer;
  function ru8(ofs:integer):byte;
  begin
    rfs.Position:=ReadOffset+ofs;
    rfs.ReadBuffer(Result,1);
  end;
  function ru16(ofs:integer):word;
  begin
    rfs.Position:=ReadOffset+ofs;
    rfs.ReadBuffer(Result,2);
  end;
  function ru32(ofs:integer):dword;
  begin
    rfs.Position:=ReadOffset+ofs;
    rfs.ReadBuffer(Result,4);
  end;
  procedure halflen;
  begin
    with PAT_PCM[Index] do begin
      Size:=Size div 2;
      setlength(Data,Size);
      LoopStart:=LoopStart div 2;
      LoopEnd:=LoopEnd div 2;
    end;
  end;
  procedure u16tos16;
  var
    cnt:integer;
    pu16:PWord;
    s:integer;
  begin
    pu16:=addr(PAT_PCM[Index].Data[0]);
    for cnt:=0 to (PAT_PCM[Index].Size div 2)-1 do begin
      s:=integer(pu16^);
      s:=s-$8000;
      pu16^:=word(s);
      inc(pu16);
    end;
  end;
  procedure s16tos12;
  var
    cnt:integer;
    ps16:Psmallint;
    s:integer;
  begin
    ps16:=addr(PAT_PCM[Index].Data[0]);
    for cnt:=0 to (PAT_PCM[Index].Size div 2)-1 do begin
      s:=integer(ps16^);
      s:=s div (1 shl 4);
      ps16^:=smallint(s);
      inc(ps16);
    end;
  end;
  procedure s16tos8;
  var
    cnt:integer;
    ps16:Psmallint;
    ps8:Pshortint;
    s:integer;
  begin
    ps16:=addr(PAT_PCM[Index].Data[0]);
    ps8:=addr(PAT_PCM[Index].Data[0]);
    for cnt:=0 to (PAT_PCM[Index].Size div 2)-1 do begin
      s:=integer(ps16^);
      s:=s shr 8;
      ps8^:=shortint(s);
      inc(ps16);
      inc(ps8);
    end;
    halflen;
  end;
  procedure u8tos8;
  var
    cnt:integer;
    pu8:Pbyte;
    ps8:Pshortint;
    s:integer;
  begin
    pu8:=addr(PAT_PCM[Index].Data[0]);
    ps8:=addr(PAT_PCM[Index].Data[0]);
    for cnt:=0 to (PAT_PCM[Index].Size div 1)-1 do begin
      s:=integer(pu8^);
      s:=s-$80;
      ps8^:=shortint(s);
      inc(pu8);
      inc(ps8);
    end;
  end;
  procedure s8tos6;
  var
    cnt:integer;
    ps8:Pshortint;
    s:integer;
  begin
    ps8:=addr(PAT_PCM[Index].Data[0]);
    for cnt:=0 to (PAT_PCM[Index].Size div 1)-1 do begin
      s:=integer(ps8^);
      s:=s div (1 shl 2);
      ps8^:=shortint(s);
      inc(ps8);
    end;
  end;
begin
  FillMemory(addr(PAT_PCM[Index]),sizeof(TPAT_PCM),0);

  with PAT_PCM[Index] do begin
    Offset:=rfs.Position;
    ReadOffset:=Offset;

    rfs.Position:=Offset+0;
    rfs.ReadBuffer(ID[0],7);
    Fractions:=ru8(7);
    Size:=ru32(8);

    LoopStart:=ru32(12);
    LoopEnd:=ru32(16);
    SampleRate:=ru16(20);
    RootFrequency:=ru32(30);
    SamplingModes:=ru8(55);

    VibSweep:=ru8(52);
    VibRatio:=ru8(53);
    VibDepth:=ru8(54);

    if VibSweep<>0 then begin
      sec:=VibSweep*6.736/$100;
      VibSweep:=sec;
    end;
    if VibRatio<>0 then begin
      sec:=38/VibRatio;
      VibRatio:=sec;
    end;

    erstr:='';
    eostr:='';

    for idx:=0 to 6-1 do begin
      tmpb:=ru8(37+idx);
      erstr:=erstr+inttohex(tmpb,2);
      tmpi:=3-((tmpb shr 6) and $03);
      tmpdw:=(tmpb and $3f)+1;
      tmpdw:=tmpdw shl tmpi; // .9 fixed point
      erstr:=erstr+'('+inttohex(tmpdw,4)+'),';
      EnvRate[idx]:=tmpdw;

      tmpb:=ru8(37+idx+6);
      eostr:=eostr+inttohex(tmpb,2);
      tmpdw:=tmpb;
      tmpdw:=tmpdw shl 8; // .8 fixed point
      eostr:=eostr+'('+inttohex(tmpdw,4)+'),';
      EnvOfs[idx]:=tmpdw;
    end;

    form1.Memo1.Lines.Add(format('Index=%d Name=%s Start=%d Size=%dbyte.',[Index,ID,Offset,Size]));
    form1.Memo1.Lines.Add(format('LoopStart=%d LoopEnd=%d',[LoopStart,LoopEnd]));
    form1.Memo1.Lines.Add(format('SampleRate=%d RootFrequency=%.3f',[SampleRate,RootFrequency/1000]));
    form1.Memo1.Lines.Add(format('Vib Sweep=%.4fms Ratio=%.4fms Depth=%d',[VibSweep*1000,VibRatio*1000,VibDepth]));
    form1.Memo1.Lines.Add(format('Fractions=%0.2x',[Fractions]));

    str:='SamplingModes='+inttostr(SamplingModes);
    if (SamplingModes and (1 shl 0))<>0 then str:=str+'/Little16bit' else str:=str+'/8bit';
    if (SamplingModes and (1 shl 1))<>0 then str:=str+'/Unsigned' else str:=str+'/Signed';
    if (SamplingModes and (1 shl 2))<>0 then str:=str+'/Looping' else str:=str+'/OneShot';
    if (SamplingModes and (1 shl 3))<>0 then str:=str+'/Pingpong';
    if (SamplingModes and (1 shl 4))<>0 then str:=str+'/Reverse';
    if (SamplingModes and (1 shl 5))<>0 then str:=str+'/Sustein';
    if (SamplingModes and (1 shl 6))<>0 then str:=str+'/Envelope';
    if (SamplingModes and (1 shl 7))<>0 then str:=str+'/Clamped?';
    form1.Memo1.Lines.Add(str);

    form1.Memo1.Lines.Add('EnvRate='+erstr);
    form1.Memo1.Lines.Add('EnvOfs ='+eostr);

    if (SamplingModes and (1 shl 0))<>0 then sm16bit:=True else sm16bit:=False;
    if (SamplingModes and (1 shl 1))<>0 then smSigned:=False else smSigned:=True;
    if (SamplingModes and (1 shl 2))<>0 then smLoop:=True else smLoop:=False;
    if (SamplingModes and (1 shl 3))<>0 then smPing:=True else smPing:=False;
    if (SamplingModes and (1 shl 6))<>0 then smEnvelope:=True else smEnvelope:=False;

    setlength(Data,Size);
    rfs.Position:=Offset+PAT_PCMHeaderSize;
    rfs.ReadBuffer(Data[0],Size);
    rfs.Position:=Offset+PAT_PCMHeaderSize+Size;

    if (sm16bit=True) and (smSigned=False) then begin // u16
      u16tos16;
      smSigned:=True;
    end;
    if (sm16bit=True) and (smSigned=True) then begin // s16
      if PCM8Bit=False then begin
//        s16tos12;
        end else begin
        s16tos8;
        sm16bit:=False;
      end;
    end;
    if (sm16bit=False) and (smSigned=False) then begin // u8
      u8tos8;
      smSigned:=True;
    end;
    if (sm16bit=False) and (smSigned=True) then begin // s8
//      s8tos6;
    end;


    form1.Memo1.Lines.Add('');
  end;
end;

procedure LoadPatch(patfn:string);
var
  rfs:TFileStream;
  cnt:integer;
begin
  rfs:=TFileStream.Create(patfn,fmOpenRead);

  LoadPATHeader(rfs);

//  PAT_Header.NumberOfSamples:=1;

  rfs.Position:=$ef;

  setlength(PAT_PCM,PAT_Header.NumberOfSamples);

  for cnt:=0 to PAT_Header.NumberOfSamples-1 do begin
    LoadPATPCM(rfs,cnt);
  end;

  rfs.Free;
end;

procedure CopyPatch2Program(patfn:string;prgidx,patidx:integer;amp,note,pan:integer;tune:double);
var
  idx:integer;
  srcsize,dstsize:integer;
begin
  if tune<>0 then begin
    Form1.Memo1.Lines.Add(format('%s,%d %f,%d,%f',[patfn,prgidx,tune,PAT_PCM[patidx].RootFrequency,PAT_PCM[patidx].RootFrequency*Power(2,-tune/12)]));
    PAT_PCM[patidx].RootFrequency:=trunc(PAT_PCM[patidx].RootFrequency*Power(2,-tune/12)+0.5);
  end;

  ProgramMap[prgidx][patidx].patfn:=patfn;

  ProgramMap[prgidx][patidx].SampleRate:=PAT_PCM[patidx].SampleRate;
  ProgramMap[prgidx][patidx].RootFreq:=trunc(PAT_PCM[patidx].RootFrequency/1000*$10000); // fix16.16
  ProgramMap[prgidx][patidx].MasterVolume:=PAT_Header.MasterVolume;
  ProgramMap[prgidx][patidx].Length:=PAT_PCM[patidx].Size;

  ProgramMap[prgidx][patidx].FractionStart:=PAT_PCM[patidx].Fractions and $0f;
  ProgramMap[prgidx][patidx].FractionEnd:=(PAT_PCM[patidx].Fractions shr 4) and $0f;

  ProgramMap[prgidx][patidx].LoopStart:=PAT_PCM[patidx].LoopStart;
  ProgramMap[prgidx][patidx].LoopEnd:=PAT_PCM[patidx].LoopEnd;

  if PAT_PCM[patidx].sm16bit=True then ProgramMap[prgidx][patidx].s16Flag:=cTrue else ProgramMap[prgidx][patidx].s16Flag:=cFalse;
  if PAT_PCM[patidx].smLoop=True then ProgramMap[prgidx][patidx].LoopFlag:=cTrue else ProgramMap[prgidx][patidx].LoopFlag:=cFalse;
  if PAT_PCM[patidx].smEnvelope=True then ProgramMap[prgidx][patidx].EnvelopeFlag:=cTrue else ProgramMap[prgidx][patidx].EnvelopeFlag:=cFalse;

  ProgramMap[prgidx][patidx].Amp:=amp;
  ProgramMap[prgidx][patidx].Note:=note;
  ProgramMap[prgidx][patidx].Pan:=pan;

  if PAT_PCM[patidx].VibSweep=0 then begin
    ProgramMap[prgidx][patidx].VibSweep:=0;
    end else begin
    ProgramMap[prgidx][patidx].VibSweep:=trunc(1/PAT_PCM[patidx].VibSweep*$10000);
  end;
  if PAT_PCM[patidx].VibRatio=0 then begin
    ProgramMap[prgidx][patidx].VibRatio:=0;
    end else begin
    ProgramMap[prgidx][patidx].VibRatio:=trunc(1/PAT_PCM[patidx].VibRatio*$10000);
  end;
  ProgramMap[prgidx][patidx].VibDepth:=trunc((Power(1.000903,PAT_PCM[patidx].VibDepth)-1)*$10000);

  for idx:=0 to 6-1 do begin
    ProgramMap[prgidx][patidx].EnvRate[idx]:=PAT_PCM[patidx].EnvRate[idx];
    ProgramMap[prgidx][patidx].EnvOfs[idx]:=PAT_PCM[patidx].EnvOfs[idx];
  end;

  srcsize:=PAT_PCM[patidx].Size;
  dstsize:=srcsize;
  if PAT_PCM[patidx].sm16bit=True then inc(dstsize,2) else inc(dstsize,1);

  ProgramMap[prgidx][patidx].DataCount:=dstsize;
  setlength(ProgramMap[prgidx][patidx].Data,dstsize);
  FillMemory(addr(ProgramMap[prgidx][patidx].Data[0]),dstsize,0);

  copymemory(addr(ProgramMap[prgidx][patidx].Data[0]),addr(PAT_PCM[patidx].Data[0]),srcsize);

  with ProgramMap[prgidx][patidx] do begin
    if PAT_PCM[patidx].sm16bit=True then begin
      Data[DataCount-1]:=Data[DataCount-2];
      end else begin
      Data[DataCount-2]:=Data[DataCount-4];
      Data[DataCount-1]:=Data[DataCount-3];
    end;
  end;

end;

const StripMode_None=0;
const StripMode_Loop=1;
const StripMode_Env=2;
const StripMode_Tail=3;

procedure ProcStrip(sm:integer);
var
  patidx:integer;
begin
  patidx:=0;

  case sm of
    StripMode_None: begin
    end;
    StripMode_Loop: begin
      PAT_PCM[patidx].smLoop:=False;
    end;
    StripMode_Env: begin
      PAT_PCM[patidx].smEnvelope:=False;
    end;
    StripMode_Tail: begin
      with PAT_PCM[patidx] do begin
        Size:=LoopEnd;
        setlength(Data,Size);
      end;
    end;
  end;
end;

type
  PLargeByteArray = ^TLargeByteArray;
  TLargeByteArray = array[0..65536*1000] of byte;

type
  PLargeWordArray = ^TLargeWordArray;
  TLargeWordArray = array[0..65536*1000] of word;

procedure ProcPingFlag;
var
  size:integer;
  patidx:integer;
  smptop,smpcnt,smpidx:dword;
  ps16:PLargeWordArray;
  ps8:PLargeByteArray;
begin
  patidx:=0;

  if PAT_PCM[patidx].smPing=False then exit;
  if PAT_PCM[patidx].smLoop=False then exit;

  smptop:=PAT_PCM[patidx].LoopStart;
  smpcnt:=PAT_PCM[patidx].LoopEnd-smptop;

  size:=smptop+(smpcnt*2);

  setlength(PAT_PCM[patidx].Data,size);

  ps16:=addr(PAT_PCM[patidx].Data[smptop]);
  ps8:=addr(PAT_PCM[patidx].Data[smptop]);

  if PAT_PCM[patidx].sm16bit=False then begin
    for smpidx:=0 to smpcnt-1 do begin
      ps8[smpcnt+smpidx]:=ps8[smpcnt-1-smpidx];
    end;
    end else begin
    for smpidx:=0 to smpcnt div 2-1 do begin
      ps16[(smpcnt div 2)+smpidx]:=ps16[(smpcnt div 2)-1-smpidx];
    end;
  end;

  PAT_PCM[patidx].Size:=size;
  PAT_PCM[patidx].LoopEnd:=size;

  PAT_PCM[patidx].smPing:=False;
end;

procedure ProcPCMHalfRate(patidx:integer);
var
  idx:integer;
  pss8,pds8:Pshortint;
  s:integer;
begin
  if PAT_PCM[patidx].sm16bit=False then begin
    pss8:=addr(PAT_PCM[patidx].Data[0]);
    pds8:=addr(PAT_PCM[patidx].Data[0]);
    for idx:=0 to (PAT_PCM[patidx].Size div 2)-1 do begin
      s:=integer(pss8^);
      inc(pss8);
      s:=s+integer(pss8^);
      inc(pss8);
      s:=s div 2;
      pds8^:=shortint(s);
      inc(pds8);
    end;

    with PAT_PCM[patidx] do begin
      Size:=Size div 2;
      setlength(Data,Size);

      LoopStart:=LoopStart div 2;
      LoopEnd:=LoopEnd div 2;
      SampleRate:=SampleRate div 2;
      Fractions:=(Fractions and $ee) shr 1;
    end;

    end else begin
    ShowMessage('not impliment PCMHalfRate for 16bitPCM');
  end;

end;

procedure LoadINI(inifn:string);
var
  prgidx,patidx:integer;
  initf:TextFile;
  str:string;
  patfn:string;
  amp,note,pan:integer;
  tune:double;
  sm:integer;
  function LoadAttr(str:string):boolean;
  var
    pos:integer;
    function GetExtInt(name:string;defint:integer):integer;
    var
      pos:integer;
      ExtStr:string;
    begin
      Result:=defint;
      name:=name+'=';
      pos:=ansipos(name,ansilowercase(str));
      if pos=0 then exit;
      ExtStr:=copy(str,pos+length(name),255);
      pos:=ansipos(' ',ExtStr);
      if pos<>0 then ExtStr:=copy(ExtStr,0,pos-1);
      Result:=strtointdef(ExtStr,defint);
    end;
    function GetExtDouble(name:string;defint:double):double;
    var
      pos:integer;
      ExtStr:string;
    begin
      Result:=defint;
      name:=name+'=';
      pos:=ansipos(name,ansilowercase(str));
      if pos=0 then exit;
      ExtStr:=copy(str,pos+length(name),255);
      pos:=ansipos(' ',ExtStr);
      if pos<>0 then ExtStr:=copy(ExtStr,0,pos-1);
      Result:=StrToFloatDef(ExtStr,defint);
    end;
  begin
    sm:=0;

    if ansipos('strip=loop',ansilowercase(str))<>0 then sm:=StripMode_Loop;
    if ansipos('strip=env',ansilowercase(str))<>0 then sm:=StripMode_Env;
    if ansipos('strip=tail',ansilowercase(str))<>0 then sm:=StripMode_Tail;

    pos:=ansipos(' ',str);
    prgidx:=strtointdef(copy(str,1,pos-1),-1);
    if prgidx=-1 then begin
      Result:=False;
      exit;
    end;

    str:=copy(str,pos+1,length(str));

    pos:=ansipos(' ',str);
    if pos=0 then begin
      patfn:=str;
      str:='';
      end else begin
      patfn:=ansilowercase(copy(str,1,pos-1));
      str:=copy(str,pos+1,length(str));
    end;

    if ExtractFileExt(patfn)<>'.pat' then patfn:=patfn+'.pat';
    patfn:=ansilowercase(patfn);

    amp:=GetExtInt('amp',0);
    note:=GetExtInt('note',0);
    pan:=GetExtInt('pan',101);
    tune:=GetExtDouble('tune',0);

    Result:=True;
  end;
begin
  AssignFile(initf,inifn);
  Reset(initf);

  while(EOF(initf)=False) do begin
    Readln(initf,str);
    if (str<>'') and (copy(str,1,1)<>'#') then begin
      if LoadAttr(str)=True then begin
        form1.Memo1.Lines.Add('-----------------------------------------');
        form1.Memo1.Lines.Add(format('prgidx=%d patfn=%s amp=%d note=%d pan=%d',[prgidx,patfn,amp,note,pan]));
        LoadPatch(GusPath+patfn);
        ProgramMapPatCount[prgidx]:=PAT_Header.NumberOfSamples;
        for patidx:=0 to ProgramMapPatCount[prgidx]-1 do begin
          ProcStrip(sm);
          ProcPingFlag;
          if PCMHalfRate=True then begin
            ProcPCMHalfRate(patidx);
          end;
          CopyPatch2Program(patfn,prgidx,patidx,amp,note,pan,tune);
        end;
      end;
    end;

  end;

  CloseFile(initf);
end;

var
  PanpotList:array[0..128] of word;

procedure CreateTonePanpotList;
var
  cnt:integer;
begin
  for cnt:=0 to 128-1 do begin
    PanpotList[cnt]:=64;
  end;
end;

procedure CreateDrumPanpotList;
var
  cnt:integer;
begin
  for cnt:=0 to 128-1 do begin
    PanpotList[cnt]:=64;
  end;

  PanpotList[35]:=64;
  PanpotList[36]:=64;
  PanpotList[37]:=64;
  PanpotList[38]:=64;
  PanpotList[39]:=54;
  PanpotList[40]:=64;
  PanpotList[41]:=34;
  PanpotList[42]:=84;
  PanpotList[43]:=46;
  PanpotList[44]:=84;
  PanpotList[45]:=58;
  PanpotList[46]:=84;
  PanpotList[47]:=70;
  PanpotList[48]:=82;
  PanpotList[49]:=84;
  PanpotList[50]:=94;
  PanpotList[51]:=44;
  PanpotList[52]:=44;
  PanpotList[53]:=44;
  PanpotList[54]:=74;
  PanpotList[55]:=54;
  PanpotList[56]:=84;
  PanpotList[57]:=44;
  PanpotList[58]:=29;
  PanpotList[59]:=44;
  PanpotList[60]:=99;
  PanpotList[61]:=99;
  PanpotList[62]:=39;
  PanpotList[63]:=39;
  PanpotList[64]:=44;
  PanpotList[65]:=84;
  PanpotList[66]:=84;
  PanpotList[67]:=29;
  PanpotList[68]:=29;
  PanpotList[69]:=29;
  PanpotList[70]:=24;
  PanpotList[71]:=99;
  PanpotList[72]:=99;
  PanpotList[73]:=94;
  PanpotList[74]:=94;
  PanpotList[75]:=84;
  PanpotList[76]:=99;
  PanpotList[77]:=99;
  PanpotList[78]:=44;
  PanpotList[79]:=44;
  PanpotList[80]:=24;
  PanpotList[81]:=24;
end;

{$L ttacenc_static.obj}

function _ttacenc_static_ExecuteEncode(pSourceSamples:PByte;SourceSamplesCount:integer;BitsPerSample:integer;pCodeBuf:PByte):integer; cdecl; external;

const WAVEID:dword=$45564157;
const TTACID:dword=$43415454;

var
  TTAC_CompressedEffectSize:integer;

procedure wbuf_ttac(wfs:TFileStream;data:array of byte;datasize:integer;bps:integer);
var
  SourceSamples:array of integer;
  SourceSamplesCount:integer;
  CodeBuf:array of byte;
  CodeBufSize:integer;
  ps8:Pshortint;
  ps16:Psmallint;
  idx:integer;
  tmp32:dword;
begin
  if False then begin
    tmp32:=WAVEID;
    wfs.WriteBuffer(tmp32,4);
    wfs.WriteBuffer(datasize,4);
    wfs.WriteBuffer(data[0],datasize);
    exit;
  end;

  if bps=8 then begin
    ps8:=addr(data[0]);
    SourceSamplesCount:=datasize div 1;
    setlength(SourceSamples,SourceSamplesCount);
    for idx:=0 to SourceSamplesCount-1 do begin
      SourceSamples[idx]:=integer(ps8^);
      inc(ps8);
    end;
    end else begin
    ps16:=addr(data[0]);
    SourceSamplesCount:=datasize div 2;
    setlength(SourceSamples,SourceSamplesCount);
    for idx:=0 to SourceSamplesCount-1 do begin
      SourceSamples[idx]:=integer(ps16^);
      inc(ps16);
    end;
  end;

  setlength(CodeBuf,SourceSamplesCount*4);
  CodeBufSize:=_ttacenc_static_ExecuteEncode(addr(SourceSamples[0]),SourceSamplesCount,bps,addr(CodeBuf[0]));
  CodeBufSize:=(CodeBufSize+3) and not 3;

  if (4+4+datasize)<(4+4+4+CodeBufSize) then begin
    tmp32:=WAVEID;
    wfs.WriteBuffer(tmp32,4);
    wfs.WriteBuffer(datasize,4);
    wfs.WriteBuffer(data[0],datasize);
    end else begin
    inc(TTAC_CompressedEffectSize,datasize-(8+CodeBufSize));
    tmp32:=TTACID;
    wfs.WriteBuffer(tmp32,4);
    wfs.WriteBuffer(CodeBufSize,4);
    wfs.WriteBuffer(SourceSamplesCount,4);
    wfs.WriteBuffer(CodeBuf[0],CodeBufSize);
  end;
end;

type
  TPCMBody=record
    PatchFilename:string;
    PatchIndex:integer;
    Offset:dword;
    LoopStart,LoopEnd,LoopFlag:dword;
    FractionStart,FractionEnd:dword;
  end;

var
  PCMBody:array of TPCMBody;
  PCMBodyCount:integer;
  PCMBody_wfs:TFileStream;

procedure InitPCMBody;
begin
  PCMBody_wfs:=TFileStream.Create(pcmfn,fmCreate);

  PCMBodyCount:=0;
end;

procedure FreePCMBody;
var
  idx:integer;
  str:string;
begin
  for idx:=0 to PCMBodyCount-1 do begin
    with PCMBody[idx] do begin
      if LoopFlag=cTrue then begin
        str:=format('%6d,%6d,%6d %x%x %s,%d',[LoopEnd-LoopStart,LoopStart,LoopEnd,FractionStart,FractionEnd,PatchFilename,PatchIndex]);
        Form1.ListBox2.Items.Add(str);
      end;
    end;
  end;
  Form1.ListBox2.Sorted:=True;
  Form1.ListBox2.Items.SaveToFile('a');

  PCMBody_wfs.Free;
end;

function StorePCMBody(patfn:string;patidx:integer;s16Flag:dword;Data:array of byte;DataCount:dword;_LoopStart,_LoopEnd,_LoopFlag,_FractionStart,_FractionEnd:dword):dword;
var
  idx:integer;
begin
  for idx:=0 to PCMBodyCount-1 do begin
    with PCMBody[idx] do begin
      if (PatchFilename=patfn) and (PatchIndex=patidx) then begin
        Result:=Offset;
        exit;
      end;
    end;
  end;

  setlength(PCMBody,PCMBodyCount+1);
  with PCMBody[PCMBodyCount] do begin
    PatchFilename:=patfn;
    PatchIndex:=patidx;
    Offset:=PCMBody_wfs.Position;
    LoopStart:=_LoopStart;
    LoopEnd:=_LoopEnd;
    LoopFlag:=_LoopFlag;
    FractionStart:=_FractionStart;
    FractionEnd:=_FractionEnd;
    Result:=Offset;
  end;
  inc(PCMBodyCount);

  if s16Flag=cFalse then begin
    wbuf_ttac(PCMBody_wfs,Data,DataCount,8);
    end else begin
    wbuf_ttac(PCMBody_wfs,Data,DataCount,16);
  end;
end;

procedure WritePSL(pslfn:string);
var
  wfs:TFileStream;
  prgidx,patidx,patcount:integer;
  ofs:array[0..128] of dword;
  envidx:integer;
  patbuf:array of dword;
  patbufsize:dword;
  procedure w32(dw:dword);
  begin
    wfs.WriteBuffer(dw,4);
  end;
  procedure wbuf8(data:array of byte;datasize:integer);
  begin
    wfs.WriteBuffer(data[0],datasize);
  end;
  procedure wpat32(dw:dword);
  begin
    patbuf[patbufsize]:=dw;
    inc(patbufsize);
  end;
  procedure wpatclear;
  begin
    patbufsize:=0;
  end;
  procedure wpatwrite;
  begin
    wfs.WriteBuffer(patbuf[0],patbufsize*4);
  end;
begin
  setlength(patbuf,16*64*4);

  wfs:=TFileStream.Create(pslfn,fmCreate);

  for prgidx:=0 to 128-1 do begin
    ofs[prgidx]:=0;
    wfs.WriteBuffer(ofs[prgidx],4);
  end;

  for prgidx:=0 to 128-1 do begin
    patcount:=ProgramMapPatCount[prgidx];
    if patcount<>0 then begin
      ofs[prgidx]:=wfs.Position;

      wpatclear;

      wpat32(patcount);

      for patidx:=0 to patcount-1 do begin
        with ProgramMap[prgidx][patidx] do begin
          wpat32(SampleRate);
          wpat32(RootFreq);
          if Amp=0 then begin
            wpat32(MasterVolume);
            end else begin
            wpat32((MasterVolume*Amp) div 100);
          end;
          wpat32(FractionStart);
          wpat32(FractionEnd);
          wpat32(Length);
          wpat32(LoopStart);
          wpat32(LoopEnd);
          wpat32(LoopFlag);
          wpat32(s16Flag);
          if Pan=101 then begin
            wpat32(PanpotList[prgidx]);
            end else begin
            wpat32(((Pan*64) div 100)+64);
          end;
          wpat32(Note);
          wpat32(VibSweep);
          wpat32(VibRatio);
          wpat32(VibDepth);
          wpat32(EnvelopeFlag);
          for envidx:=0 to 6-1 do begin
            wpat32(EnvRate[envidx]);
          end;
          for envidx:=0 to 6-1 do begin
            wpat32(EnvOfs[envidx]);
          end;

          wpat32(StorePCMBody(patfn,patidx,s16Flag,Data,DataCount,LoopStart,LoopEnd,LoopFlag,FractionStart,FractionEnd));
        end;

      end;

      wpatwrite;
    end;
  end;

  wfs.Position:=0;

  for prgidx:=0 to 128-1 do begin
    w32(ofs[prgidx]);
  end;

  wfs.Free;
end;

procedure DeletePSLs;
var
  idx:integer;
begin
  for idx:=0 to 128-1 do begin
    DeleteFile(ExtractFilePath(Application.ExeName)+'sftone'+inttostr(idx)+'.psl');
    DeleteFile(ExtractFilePath(Application.ExeName)+'sfdrum'+inttostr(idx)+'.psl');
  end;
end;

procedure MakeBIN(dstfn:string);
var
  wfs:TFileStream;
  fn:string;
  idx:integer;
  toneofs,drumofs:array[0..128] of dword;
  pcmofs:dword;
  procedure wdw(dw:dword);
  begin
    wfs.WriteBuffer(dw,4);
  end;
  procedure Padding;
  var
    ofs:integer;
    dw:dword;
  begin
    ofs:=wfs.Position;
    ofs:=ofs and $3;
    ofs:=4-ofs;
    if (ofs mod 4)<>0 then begin
      dw:=0;
      wfs.WriteBuffer(dw,ofs);
    end;
  end;
  function WriteFile(fn:string):integer;
  var
    rfs:TFileStream;
    buf:array of byte;
    size:integer;
  begin
    if FileExists(fn)=False then begin
      Result:=0;
      exit;
    end;

    Result:=wfs.Size;

    rfs:=TFileStream.Create(fn,fmOpenRead);
    size:=rfs.Size;
    setlength(buf,size);
    rfs.ReadBuffer(buf[0],size);
    rfs.Free;

    wfs.WriteBuffer(buf[0],size);

    Padding;
  end;
begin
  wfs:=TFileStream.Create(dstfn,fmCreate);

  wdw($31534650);

  wfs.Position:=4;

  for idx:=0 to 128-1 do begin
    wdw(0);
  end;
  for idx:=0 to 128-1 do begin
    wdw(0);
  end;

  wdw(0);

  for idx:=0 to 128-1 do begin
    fn:=ExtractFilePath(Application.ExeName)+'sftone'+inttostr(idx)+'.psl';
    toneofs[idx]:=WriteFile(fn);
  end;
  for idx:=0 to 128-1 do begin
    fn:=ExtractFilePath(Application.ExeName)+'sfdrum'+inttostr(idx)+'.psl';
    drumofs[idx]:=WriteFile(fn);
  end;

  pcmofs:=WriteFile(pcmfn);

  wfs.Position:=4;

  for idx:=0 to 128-1 do begin
    wdw(toneofs[idx]);
  end;
  for idx:=0 to 128-1 do begin
    wdw(drumofs[idx]);
  end;

  wdw(pcmofs);

  wfs.Free;
end;

procedure TForm1.CreateBIN(binfn,prefn:string;_PCM8Bit,_PCMHalfRate:boolean);
var
  idx:integer;
  fn,mapfn,pslfn:string;
  fs:TFileStream;
  FileSize:integer;
  procedure ProcTone(mapfn,pslfn:string);
  begin
    if FileExists(mapfn)=False then exit;
    memo1.Lines.Add(mapfn+'--------------------------------------------------------');
    InitProgramMap;
    CreateTonePanpotList;
    LoadINI(mapfn);
    WritePSL(pslfn);
  end;
  procedure ProcDrum(mapfn,pslfn:string);
  begin
    if FileExists(mapfn)=False then exit;
    memo1.Lines.Add(mapfn+'--------------------------------------------------------');
    InitProgramMap;
    CreateDrumPanpotList;
    LoadINI(mapfn);
    WritePSL(pslfn);
  end;
begin
  TTAC_CompressedEffectSize:=0;

  PCM8Bit:=_PCM8Bit;
  PCMHalfRate:=_PCMHalfRate;

  memo1.Clear;

  GusPath:=ExtractFilePath(Application.ExeName)+'GUS\';

  DeletePSLs;
  DeleteFile(pcmfn);

  InitPCMBody;

  for idx:=0 to 128-1 do begin
    fn:='sftone'+inttostr(idx);
    mapfn:=ExtractFilePath(Application.ExeName)+prefn+'\'+fn+'.map';
    pslfn:=ExtractFilePath(Application.ExeName)+fn+'.psl';
    ProcTone(mapfn,pslfn);
  end;
  for idx:=0 to 128-1 do begin
    fn:='sfdrum'+inttostr(idx);
    mapfn:=ExtractFilePath(Application.ExeName)+prefn+'\'+fn+'.map';
    pslfn:=ExtractFilePath(Application.ExeName)+fn+'.psl';
    ProcDrum(mapfn,pslfn);
  end;

  FreePCMBody;

  MakeBIN(ExtractFilePath(Application.ExeName)+binfn);

  DeletePSLs;
  DeleteFile(pcmfn);

  fs:=TFileStream.Create(ExtractFilePath(Application.ExeName)+binfn,fmOpenRead);
  FileSize:=fs.Size;
  fs.Free;

  ListBox1.Items.Add(binfn+' '+format('TAC_CompressedEffectSize=%dbyte (%f%% reduced.)',[TTAC_CompressedEffectSize,TTAC_CompressedEffectSize/(TTAC_CompressedEffectSize+FileSize)*100]));
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
  CreateBIN('midrcp_default.bin','sfmini',True,True);
{
  Application.Terminate;
  exit;
}

  CreateBIN('midrcp_sf16bit_gmmap.bin','sfgm',False,False);
  CreateBIN('midrcp_sf16bit_sc88promap.bin','sfsc88pro',False,False);

  CreateBIN('midrcp_sf8bit_gmmap.bin','sfgm',True,False);
  CreateBIN('midrcp_sf8bit_sc88promap.bin','sfsc88pro',True,False);
end;

end.












