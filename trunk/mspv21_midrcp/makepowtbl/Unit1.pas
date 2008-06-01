unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Math;

type
  TForm1 = class(TForm)
    Memo1: TMemo;
    procedure FormCreate(Sender: TObject);
  private
    { Private êÈåæ }
  public
    { Public êÈåæ }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

procedure TForm1.FormCreate(Sender: TObject);
var
  wfs:TFileStream;
  pow:integer;
  note:integer;
begin
  wfs:=TFileStream.Create('powtbl.bin',fmCreate);

  for note:=0 to 12*1024-1 do begin
    pow:=trunc(Power(2,note/(12*1024))*$10000)-$10000;
    memo1.Lines.Add(format('%d %x %.12f',[note,pow,pow/$10000]));
    wfs.WriteBuffer(pow,2);
  end;

  wfs.Free;

  wfs:=TFileStream.Create('sintbl.bin',fmCreate);

  for note:=0 to 120*1-1 do begin
    pow:=trunc(sin(note/(120*1)*PI*2)*$10000);
    memo1.Lines.Add(format('%d %.12f',[note,pow/$10000]));
    wfs.WriteBuffer(pow,4);
  end;

  wfs.Free;
end;

end.
