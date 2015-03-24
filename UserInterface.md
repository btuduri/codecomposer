**PALib 와 debkitARM을 low API는 서로 병행할 수 없다!!**

만약 PALib로 화면에 뭔가를 그린다고 하면 그냥 PALib 함수를 죄다 이용해서 그려야지 openGL API로 그린것을 vram에 다가 덮어 쓸 수 없는 듯하다.
만약 OpenGL을 사용한다면 PALib 함수로 VRAM을 초기화하고 관리하는 함수를 사용하면 안되고 devkit 의 low API를 이용해야만한다. 이 경우 레퍼런스도 없고 시간도 없고 여러모로 힘들것 같으니깐 일단 PALib로 개발하기로 한다.


---


## 1. 전체 클래스의 구조 ##


## 2. API ##

### 3D Sprite Function ###

  * PA\_3DSetSpriteWidthHeight(u16 spriteNum, u16 widthSize, u16 heightSize);
    * 이미지의 가로/세로 크기를 지정할 수 있다.

  * 8bit 배경 이미지 읽어들이기
    * 배경을 pcx 변환시 8bit로 설정후 변경한다..
```
PA_Init8bitBg(0,3);
PA_Load8bitBgPal(0,(void*)Background_Pal);  
PA_Load8bitBitmap(0,Background_Bitmap);
```

  * 16bit 배경 이미지 읽어들이기
    * 배경을 PAGfx 변환시 16bit로 설정 후 변경한다.
```
PA_Init16bitBg(0, 3);
PA_Load16bitBitmap(0, Background_Bitmap);
```

  * PA\_3DCreateSpriteFromTex(sprite#, texture#, width, height, palete#, posX, posY);
    * 텍스쳐를 바탕으로 스프라이트를 생성한다. 이 함수의 호출과 동시에 화면이 그려진다. 나중에 스프라이트를 지우라는 명령을 하지 않으면 계속 스프라이트가 화면에 그려진다.
    * 생성후 이 함수는 만들어진 텍스쳐 스프라이트를 구분하기위한 인식번호를 리턴한다. u16타입이며 0번부터 만들어져서 1, 2, 3... 순으로 발급하는 것을 확인했다.

  * sprite number라는 것에 집중할 필요가 있다. sprite number는 일종의 sprite priority다. 이 값을 기준으로 화면에 스프라이트가 찍히는 우선순위를 결정한다.

  * PA\_3DDeleteSprite( sprite# );
    * 해당 스프라이트를 화면에서 지워주는 함수다.

### Font Sprite Function ###

  * PA\_OutputSimpleText(bool screen, u16 x, u16 y, const char **text)
    * 가장 쉽게 그리고 간단하게 글자를 출력하는 함수, x,y는 타일 인덱스를 말하는 것으로 x는 타일(0-31)에서 x좌표이다. y는 타일(0-23)에서 y좌표이다. 텍스트를 삭제할려면 공백을 가지고 덮어 쓰면 된다. PA\_OutputSimpleText(0, 0, 0, " “);**

  * PA\_OutputText(bool screen, u16 x, u16 y, const char **text, 변수들..)
    * 변수와 함께 문자를 출력하는데 사용된다.
    * int형 : PA\_OutputText(1,0,0,"Stylus X : %d   Stylus Y : %d", Stylus.X, Stylus.Y);
    * float형 : PA\_OutputText(1,0,0,"Float value : %f3", test);
    * string형 : PA\_OutputText(1,0,0,"Hi %s", name); // char name[100](100.md);
    * %cX(0~9) : 이 첨자를 문자열에 붙여 문자를 특정 색상으로 바꾼다. (0 = white, 1 = red, 2 = green, 3 = blue, 4 = purple, 5 = cyan, 6 = yellow, 7 = light grey, 8 = dark grey, 9 = black) ex) PA\_OutputText(0, 0, 0, "Color test...%c1another one, %c2again, %c3again...");**

  * A\_SetTextCol(screen, r, g, b);
    * 스크린상의 **모든 문자**를 같은 색으로 바꾼다.
    * Blue : PA\_SetTextCol(screen, 0, 0, 31)
    * ed : PA\_SetTextCol(screen, 31, 0, 0)
    * hite : PA\_SetTextCol(screen, 31, 31, 31)
    * lack : PA\_SetTextCol(screen, 0, 0, 0)
    * rey : PA\_SetTextCol(screen, 22, 22, 22)
    * agenta : PA\_SetTextCol(screen, 31, 0, 31)

  * PA\_SetTextTileCol(screen, colorNumber);
    * PA\_SetTextCol 과 다른 점은 전에 설정된 텍스트 색은 변경하지 않는다.
    * 0 = white, 1 = red, 2 = green, 3 = blue, 4 = purple, 5 = cyan, 6 = yellow, 7 = light grey, 8 = dark grey, 9 = black

  * PA\_InitCustomText(screen#, background#, newFontName );
    * 사용자 지정폰트를 사용할 수 있다.
    * newFontName은 폰트를 스프라이트로 떠서 pcx로 변환한 이미지를 말하는 듯하다

  * PA\_16cText(screen#, x1, y1, x2, y2, textString, color (1-10), text size (0-4), MaxCharNum);
    * 텍스트를 그리는데 크게 두가지 옵션이 있다. 16c 와 8bit 모드가 그것이다. 16c는 16color만을 지원하는 것으로 스크린에 그리거나 지울때 8bit 보다 2-4배 빠르다. Vram의 거의 반절정도의 공간만 사용한다. 8bit : 색상을 256 color를 지원한다. 쓰여지는 글자를 좌,우로 회전할수 있다.
    * 사용하기 전에 PA\_Init16cBg(screen#, background#); 으로 초기화 한다.

### Input Function ###

  * NDS로의 입력은 패드, 스타일러스, 키보드가 지원된다.
  * 패드
    * 매 프레임마다 자동으로 상태값이 업데이트 되기때문에 컨트롤 하기가 쉽다.
    * 키 패드가 눌러진 상태는 다음 3가지가 있다.
      * Held : 키가 눌러진 상태다
      * Released : 키가 눌러졌다 때졌다.
      * Newpress : 키가 눌러졌다.
    * 키 종류
      * A, B, X, Y, L, R, Start, Select
```
if(Pad.Held.Up)
{
    MoveUp();
}
if(Pad.Held.Down)
{
    MoveDown();
```

  * 스타일러스
    * 패드와 매우 비슷하다. 스타일러스 위치로 X, Y를 사용한다
```
if (Stylus.Held)
{
  PA_SetSpriteXY(screen, sprite, Stylus.X, Stylus.Y);
}
```

  * 키보드는 사용되지 않을 것이므로 생략
  * 입력에 대한 예제 코드
```
PA_Init();    // Initializes PA_Lib
PA_InitVBL(); // Initializes a standard VBL

PA_InitText(1, 0);  // Initialise the text system

PA_InitKeyboard(2); // Load the keyboard on background 2...

PA_KeyboardIn(20, 100); // This scrolls the keyboard from the bottom, until it's at the right position

PA_OutputSimpleText(1, 7, 10, "Text : ");

s32 nletter = 0; // Next letter to right. 0 since no letters are there yet
char letter = 0; // New letter to write.
char text[200];  // This will be our text.

// Infinite loop to keep the program running
while (1)
{
// We'll check first for color changes, with A, B, and X
if (Pad.Newpress.A) PA_SetKeyboardColor(0, 1); // Blue and Red
if (Pad.Newpress.B) PA_SetKeyboardColor(1, 0); // Red and Blue
if (Pad.Newpress.X) PA_SetKeyboardColor(2, 1); // Green and Red
if (Pad.Newpress.Y) PA_SetKeyboardColor(0, 2); // Blue and Green

letter = PA_CheckKeyboard();

if (letter > 31) { // there is a new letter
text[nletter] = letter;
nletter++;
}
else if ((letter == PA_BACKSPACE)&&nletter) { // Backspace pressed
nletter--;
text[nletter] = ' '; // Erase the last letter
}
else if (letter == '\n'){ // Enter pressed
text[nletter] = letter;
nletter++;
}

PA_OutputSimpleText(1, 8, 11, text); // Write the text
PA_WaitForVBL();
}
```


## 3. 기타 ##

### 텍스쳐 스프라이트 생성에 관해서 ###
  * 텍스쳐 스프라이트를 생성하면 각 스프라이트를 구별하는 식별 번호가 주어지는데 이번호를 통해 스프라이트를 이동시고 삭제할 스프라이트를 지정할 수 있다.
  * 텍스쳐 생성 순서가 중요하다. 현재 텍스쳐로 만들어진 스프라이트의 수는 최소 10개 이상이다. (악기 아이콘 8 + 악기 추가1 + Play 1 + 악기별 sheet + 음표)
  * 순서는 다음과 같다.
    * TEX\_PIANO = 0, TEX\_GUITAR, TEX\_BGUITAR, TEX\_EGUITAR, TEX\_VIOLIN, TEX\_DRUM, TEX\_FLUTE, TEX\_TRUMPET, TEX\_PLUS, TEX\_PLAY, TEX\_SHEET5, TEX\_LINE
  * 모든 아이콘의 크기는 32x32 로 고정되어있다. (예외 : 악보 시트, 라인)
  * 악보 시트의 스프라이트 크기 : 256 x 64 (실제 유효 크기 220 x 35)
  * 텍스쳐 스프라이트 식별 번호 할당 계획
    * 0 : Icon Plus
    * 1 : Icon Play
    * 2~15 : 현재 비워져 있음
    * 16~31 : IconPlus class에 할당, Icon Plus가 눌러지면 생기는 메뉴의 스프라이트를 위해 할당됨
    * 32~47 : IconPlay class에 할당. 현재 비워져 있음
    * 48~63 : 추후 추가될 메뉴를 위해 할당. 현재 비워져 있음
    * 64 ~ : Music class에 할당 되어 있음.
      * usic class는 악기 구조체를 포함하고 있는데 악기별로 64개의 가용 스프라이트를 가질 수 있음
      * Instrument에 할당된 64개의 sprite 중 첫번째는 악기 아이콘 스프라이트의 식별 번호
      * Instrument에 할당된 64개의 sprite 중 두번째는 악보 시트 스프라이트의 식별 번호
      * 실제 음표(라인)을 나타내는 스프라이트는 한 악기당 화면에 62개를 그릴 수 있음( 악보 시트의 가로 넓이 (220px) / 라인 segment 넓이 (8px) = 27.5 이므로 최대 28개의 음표 라인이 들어간다고 생각하고 넉넉하게 잡았음