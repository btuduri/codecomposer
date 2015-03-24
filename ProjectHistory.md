# Note #
  * 저 원래 GRE 시험을 5월 28일날 보려고 하다가 6월 27일, 7월 2일로 미루기로 했습니다.
  * 그래서 6월 26일 ~ 7월 3일사이에는 한국에 없습니다(?!)
  * 면세점에서 사실것 있으면 미리 여기에 적어주시면 선후불제로 하겠습니다 ㅋㅋ
  * 이번 iphone 발표에서 저희가 만들려는 어플리케이션과 비슷한것이 나왔습니다.
    * http://monsterdesign.tistory.com/462 을 참조해주세요.

# Moonshell MIDI Part #

  * 개발 내용 요약 :  NDSL에 악기 사운드 뱅크를 넣어두고, 직접 바닥부터 미디 인터페이스를 개발하는겁니다.
  * 그러나 실제로.....개발하기는 귀찮고 NDSL에서 미디를 플레이하거나 관련 부분을 찾아본 결과 Moonshell이라는 NDSL용 미뎌 플레이어 발견
  * NDSL의 커스텀 쉘처럼 되어있어서 유일하게 ndsl에서 midi 플레이가 가능한 소프트웨어. 간단한 영문 소개는 위키페디아 참조.
  * 따라서 별도로 사운드 뱅크를 플러그 인 형태로 내장시킬 수 있습니다!!!! (사실 사운드뱅크, 사운드폰트라고 부르기도 하는데 이건 공개된 자료가 많이 널려있다.
  * 사운드뱅크를 어떻게 만들것인가? 에 대한 방법은 이 링크를 참조한다. (호기심 차원)
  * "다운로드 페이지", 게다가 오픈소스! (Moonshell 전체 소스, midi 부분은 본 소스에는 없더군요)
    * 사운드뱅크에 관련된 음원 다운로드 페이지 (Moonshell 제작자가 만든 음원)
    * 미디 부분은 플러그인 형태로 들어가나봅니다. 일단은 관련 소스도 있긴 있네요 (mspv20\_midrcp, mspv21\_midrcp 디렉토리)
    * 해당소스를 이해하고, 추려내서 일단 돌려보는게 급선무.
      * 일단 Moonshell은 기본적으로 미디어플레이어이기 때문에, main.cpp에 보면 플레이어의 추상적인 인터페이스가 구현되어있습니다.
    * 플러그 인 개발 환경은 DevKitProR20+libnds-20070127이 기반임.
  * 현재 생각하고 있는 방법은
    * #1 : Moonshell의 midi 플레이어 부분을 따와서 PALIB에 집어넣은다음에 이 PALIB으로 개발을 하는 것이 좋을것 같다는 생각.
    * #2 : UI도 그냥 밑바닥 부터 모조리 만드능? ㅋㅋ 사실 이것도 그렇게 나쁘진 않다; UI만드는 사람이 죽.....

# Visual Studio 2005 에 PALib 개발환경을 이용하여 개발하기 #
  * 이 방법을 사용하기로 했었음.
  * http://0pen.us/Forum/freeforum/600 을 참조하여 개발을 진행함.
  * 예제를 따라하다가 ds\_arm9\_crt0.s:(.init+0x2ac): undefined reference to `initSystem' 발생시 http://vixen.egloos.com/3958026을 참조하여 개발을 진행한다.
  * 구지 PALib 아니여도 DevkitPro를 사용해서 Visual Studio와 연계하는 방법.
    * http://www.coderjoe.net/archive/2007/07/23/nintendo-ds-homebrew-tools-of-the-trade/
    * http://www.coderjoe.net/archive/2007/07/30/using-the-libnds-example-template-makefiles/
    * http://www.coderjoe.net/archive/2007/07/30/using-devkitarm-and-libnds-with-visual-studio-2005/

# Case1: DevkitPro 기반 개발 환경 설정하기 (Deprecated) #

- 홈브류를 사용해서 개발하기
1) VMWare가 필요한 경우,제 서버에서 받으시면 됩니다. "프로그램", "시리얼 생성기", "Ubuntu Linux" // (김형식)

2) http://sourceforge.net/project/showfiles.php?group_id=114505 에서 devkitPro Updater를 다운받아 설치하면 된다.

devkitARM, devkitPSP, devkitPPC, 등등 여러가지를 설치할 수 있으며, nds는 ARM기반이기에 devkitPSP와 devkitPPC는 체크해제한다.

설치하고 devkitPro를 설치할 때 같이 깔린 Programmer's Notepad로 examples에 있는 것들을 컴파일해보면 잘 되는걸 알 수 있다.

라고 나왔지만..



his application has requested the Runtime to terminate it in an unusual way.
Please contact the application's support team for more information.

위와 같은 런타임 에러로 엄청 고생을 했다.

이리 저리 알아봤지만, 정확한 해결방법은 보이지 않았고..(포맷하는 것도 한 방법)

하나 안 것은 cmd 로 콘솔창을 열지 말고 command로 콘솔창을 열어 make하면 잘 컴파일된다.;;

# dslinux 컴파일 (Deprecated) #

> - 직접 DSLinux를 이용하여 올리는 방법 : 질문! 이거 계속 진행하시는지요? 혹시 해결해내셨으면 내용 기록 부탁드립니다!



> 0. http://dslinux.org/cgi-bin/moin.cgi/CompilingDSLinux#head-ea3523fe98464031505df8fbe382c46b6279d895 기본적으로 여기를 참조한다.



  1. 터미널에서 다음과 같이 입력하여 자료를 다운받는다.

  1. wget http://stsp.spline.de/dslinux/dslinux-snapshot.tar.gz     (200메가 이상)
> > wget http://stsp.spline.de/dslinux/toolchain/dslinux-toolchain-2007-11-03-i686.tbz






> 2)  저는 이렇게 했습니다. (김형식)

> - 그냥 svn을 설치한담에 (이런식으로 sudo apt-get install subversion),  svn checkout svn://dslinux.spline.de/dslinux/trunk dslinux

> - wget http://stsp.spline.de/dslinux/toolchain/dslinux-toolchain-2008-01-24-i686.tbz (이게 좀더 최신버전이네요)


> 2. NDSL 디렉토리를 생성 후, 여기에 복사를 한다.



> 3. 압축을 푼다.

> tar xvzf dslinux-snapshot.tar.gz
> tar xvjf dslinux-toolchain-2007-11-03-i686.tbz



> 4. 패스 설정

> export PATH=$PATH:$HOME/NDSL/dslinux-toolchain-2007-11-03-i686/bin



> 5. make menuconfig

> 뻔한 이야기이긴하지만  이렇게 했는데 뭐 ncurses를 찾을 수 없다느니 뭐느니 하면

> apt-get install libncurses**해서 관련 라이브러리를 모조리 인스톨 합니다. (김형식)**


> Kernel/Library/Defaults Selection --> [**] Customize Kernel Settings**

> 이것을 실행하고 Exit로 끝까지 빠져나오면 이상한 것들을 계속 물어본다. (일단은 다 Yes;;) 그러고 linux kernel을 menuconfig했을 때와 비슷한 메뉴를 볼 수 있다.)

> Device Drivers --> Graphic support --> Console display driver support --> [**] Mini 6x6 font**



> 6. 터미널을 2개 이상 사용할 수 있도록 설정

> vendors/Nintendo/DLDI/inittab 파일에 tty2::linux:/usr/bin/agetty 38400 tty2 를 추가한다.



> 7. 컴파일한다. make

> (우분투의 경우 패키지 에러가 났다. sudo apt-get install 해당패키지; 로 넘어갔다. 좀 더 구체적으로 적어보자면 다음과 같다)

> - /bin/sh: bison: not found -> apt-get install bison (김형식)

> - checking for capable lex... insufficient
configure: error: Your operating system's lex is insufficient to compile
> libpcap.  flex is a lex replacement that has many advantages, including
> being able to compile libpcap.  For more information, see
> http://www.gnu.org/software/flex/flex.html. (김형식)

> -> apt-get install flex


> install.sh or install\_sh 가 없다고 에러가 나서 컴파일 중지. svn으로 최신버전 다운 받는 중..

> - svn 버전도 동일한 현상 발생


make[2](2.md): Leaving directory `/home/yy20716/workspace/dslinux/lib'
make[2](2.md): Entering directory `/home/yy20716/workspace/dslinux/lib/libtremor'
cd src && env CFLAGS="-O2 -g -fomit-frame-pointer -fno-common -fno-builtin -Wall  -mswp-byte-writes -DCONFIG\_NDS\_ROM8BIT -mcpu=arm946e-s -mfpu=fpe3 -DEMBED -DPIC -fpic -msingle-pic-base -Dlinux -Dlinux -Dunix -DuClinux" LDFLAGS="-Wl,-elf2flt -DPIC -fpic -msingle-pic-base -mswp-byte-writes" \
> ./configure --build=i386 --host=arm-linux-elf \
> --prefix=/usr --disable-shared
configure: error: cannot find install-sh or install.sh in "." "./.." "./../.."
make[2](2.md): **[.configured] 오류 1
make[2](2.md): Leaving directory `/home/yy20716/workspace/dslinux/lib/libtremor'
make[1](1.md):** [all](all.md) 오류 2
make[1](1.md): Leaving directory `/home/yy20716/workspace/dslinux/lib'
make: **[subdirs](subdirs.md) 오류 1**

# iDeaS를 이용하여 DSLinux 돌려보기 #


> 참조 : http://www.dslinux.org/f0rums/viewtopic.php?t=555&highlight=emulator+ideas

> - 기계를 몇대 사는것이 좋을것 같습니다;


# Reference Page #
Official Hompage of DSLINUX
Introduction on Wikipedia

http://www.ndsemulator.com/

http://www.ideasemu.org/



# 발생했던 문제점 #

## : PALib자체는 MIDI 지원을 하지 않습니다 ##

그래서 찾아본 자료.

- http://forum.palib.info/index.php?topic=3698.msg21767

- http://events.ccc.de/congress/2006/Fahrplan/attachments/1228-nintendo_hacking_teatime_23c3.pdf

- http://dsmidiwifi.tobw.net/index.php?cat_id=0 (홈페이지 가운데 있는 데모를 꼭 보세요)


생각해본 차선책 1. 그러나 이 방법은 채택하지 않았다.

// 위의 링크를 보고 생각이 든게 NDSL의 사운드 품질이 그렇게 썩 좋지 않아서 실제 MIDI를 사용하기 어려운게 아닌가 합니다.

// 어떻게보면 저기 YouTube에 있는 것처럼 각 NDSL마다 악기를 맡아서 합주 형식으로 음악을 만드는것도 재밌어보이긴하네요.
// 반드시 저기 프로그래밍 방식을 따라하자는 의견은 아닌데, PALib으로는 연주 인터페이스만 만들어두고 소리는 저렇게 PC에서 내게하는것도 괜찮아보입니다.  (푸하하)
// 다만 이렇게 프로젝트를 끌고 가면 인교형에게 허락을 먼저 받아야하긴하겠네요. 혹시 괜찮은 의견 있으시면 여기에 적어주셨으면 합니다.
(김형식)

# User Interface #
UI를 개발하는데 알아야할 것들을 여기에 정리해둔다.

1. 화면에 뭔가를 그리기 위해
프레임 버퍼
최종 디스플레이 장치화면에 1:1로 대응하는 일종의 비디오램 버퍼에 직접 접근할 수 있다. 다만 화소별 색상을 모두 직접 작성해야하는 어려움있다.
다시 말하면 스프라이트 이미지라는 개념도 없고, 폴리곤이라는 개념도 없이 가로, 세로 픽셀마다 최종 그려져야할 색상값을 직접 계산해서 발라주는 것이다.
강력하지만 프로그래머가 할게 엄청 많아지고 실제로 쓰일지는 의문이다.

uint16**framebuffer = ...;
> for(int i = 0; i < SCREEN\_WIDTH** SCREEN\_HEIGHT; ++i)
    * ramebuffer++ = RGB15(0,0,31);

Screen
알다시피 NDS는 2개의 스크린을 가지고 있다. 아래쪽 터치스크린을 main, 위쪽 스크린을 sub라 부른다.(물론 바뀔 수 있다.)
화면크기는 가로 x 세로 = 256 x 192 pixel이다. (0~255 & 0 ~ 191)
두 스크린 중 main 스크린에서만 3D 이미지를 그려낼 수 있다는 것을 기억하자.

void videoSetMode( videoMode ); & videoSetModeSub( videoMode)
화면에 표시되는 한개 이상의 프레임버퍼의 설정을 해주는 함수다. videoSetMode()는 메인 스크린, videoSetModeSub() 서브 스크린의 모드 설정.
예 를 들면 더블버퍼링을 지원할 것인지, page flipping을 지원할 것인지 같은 것을 설정한다.예를 들면 videoSetMode(MODE\_0\_3D);로 설정해 Background image 0번을 3D로 그려낼 수 있다.이 함수의 인자에 대해서 좀 더 자세히 이야기하면 NDS는 최대 4개까지 background image layer를 가진다. 이중에 openGL을 사용해 3D 이미지를 그릴 수 있는 것은 background 0번 뿐이다.
인자의 종류와 기능에 대해 정리한 테이블은 여기 에서 참조할 수있다.

void vramSetBankA(VRAM\_A\_LCD);
프레임버퍼로 지정된 비디오 렘은 A, B ... 같은 형식으로 이름붙여져 있다. 위의 함수는 첫번째 비디오램을 프레임버퍼로 사용하겠다는 것이다.

void lcdSwap();
상하 스크린의 main과 sub의 역할을 바꾸는 함수다.
우리 프로젝트에서 PA\_Init(); PA\_InitVBL(); 두 문장을 수행 하면 디폴트로 아래쪽 터치스크린이 main screen으로 잡히게 된다.