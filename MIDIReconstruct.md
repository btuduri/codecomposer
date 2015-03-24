# 일러두기 #
  * 현재 프로젝트는 utf-8 기반으로 작성되어있습니다.
    * 혹시 저장할때나 볼때 문제가 생기시면 http://wiz.pe.kr/tag/Visual%20Studio?page=3을 참조하시길 바랍니다.
  * SVN의 사용 방법에 대해서는 http://www.pyrasis.com/main/Subversion-HOWTO을 참조해주세요.

# 현재 진행 과정 #
## 프로젝트 생성 과정에 관하여 ##
  * 다음 블로그의 글 3개를 참조(고대로 따라?)하여 처음 프로젝트를 생성하였습니다.
    * http://www.coderjoe.net/archive/2007/07/23/nintendo-ds-homebrew-tools-of-the-trade/
    * http://www.coderjoe.net/archive/2007/07/30/using-the-libnds-example-template-makefiles/
    * http://www.coderjoe.net/archive/2007/07/30/using-devkitarm-and-libnds-with-visual-studio-2005/
    * devkitPro의 설치는 http://vision.byus.net/tt/entry/devkitPRO-PAlib-%C4%C4%C6%C4%C0%CF-%B0%A1%B4%C9%C7%D1-%B4%D1%C5%D9%B5%B5DSNDS-%B0%B3%B9%DF-%C8%AF%B0%E6-%B1%B8%C3%E0%C7%CF%B1%E2 을 참조하세요.
    * devkitPro의 경로는 C:\devkitPro 입니다.
    * 에뮬레이터의 경로는 C:\devkitPro\emulators\nosgba 입니다.
    * 이렇게 해서 기본적인 템플릿이 컴파일되고 동작하는 것을 확인하였습니다.
  * 그리고 mspv21\_midrcp에 있는 일부파일과 Moonshell 본체에 있는 일부 소스를 집어넣었습니다.
  * 라이브러리들의 위치
    * 라이브리리는 기본적으로 함수 포인터들이 구조체들에 선언되어서 연결되는 구조이다.
    * TPlugin\_StdLib에 정의되어 있으며, Dll.cpp 파일에 함수 포인터들이 정의되어 있다.
    * 해당부분을 직접 복사해서 낑겨넣어놨습니다 아직 정리가 필요합니다
    * Dll.**에 있는 파일의 구조체나 정보등은 모두 plugin\_supple.**으로 옮겨놨습니다.

## TO DO ##
  * Moonshell의 소스 분석이 어느정도 진행된 상태이므로
    * 해당 소스를 다시 조합하여 실제 Code Composer에서 사용할 부분을 만들어야합니다.
    * 그런 까닭에 SVN의 trunk/MIDISoundTest 디렉토리에 어느정도 조합을 해놔서 넣어놨습니다.
  * **문쉘은 자신만의 libnds를 따서 씁니다. 모든 소스가 이걸로 맞춰져있으므로 주의하셔야..**
    * 그래서 nds\_include라는 디렉토리에 문쉘 소스에서 퍼온 libnds가 있습니다.
      * 이 버전은 devkit에 있는 libnds와 상당히 많이 다른 까닭에 주의하셔야합니다.
      * 그런 이유로 프로젝트에서 참고하는 헤더의 경로를 C:\devkitPro\libnds가 아니라 예를 들어 이런식, D:\Workspace\MIDISountTest\nds\_include 으로 바꿔주시는것 잊지마세요.

  * GBA NDS FAT 디렉토리에 있는 소스를 모두 source 디렉토리로 옮겼습니다.
    * 현재 사용중인 Makefile을 수정하지 않는 한 현재 소스가 컴파일이 안되서 링킹시 그 부분이 undefined reference...이 발생합니다.
    * 이 부분 소스가 완전 초개판입니다 ㅠ 이 부분은 원래 문쉘도 아니여서....ㅠ


## 의문점(진행된 점, 남은 점) ##
  * PRFStart, PRFEnd 함수가 2개가 있는데, 정확히 어디다가 쓴건지, 왜쓴건지 모르겠네요.
    * http://code.google.com/p/codecomposer/source/browse/trunk/MIDISoundTest/source/_console.c#150
    * http://code.google.com/p/codecomposer/source/browse/trunk/MIDISoundTest/source/_consoleWriteLog.cpp#4
    * 소스 분석을 해보니 이 부분은 일종의 벤치마크용 타이머입니다. 필요없으니 지우셔도 ok.

  * 시작 부분에 LoadINI(GetINIData(), GetINISize()); 에서 GetINIData는 Plugin\_GetINIData 란 함수를 말한다. 이 함수는 TPluginBody 구조체의 INIData 변수를 반환한다. TPluginBody는 어디서 설정?
    * 이 부분은 Dll.cpp에 있습니다.

  * plugin.h, plugin\_def.h 부분은 일단 stdio.h 같은 것을 사용할 수 있기 때문에 컴파일시 포함이 안되게 해놨습니다만.
    * 일부 메모리 함수 부분이 추가가 안되어있습니다. 이 부분 정리부탁드립니다.
      * memtool.h 와 memtool.cpp를 추가하고 컴파일 하면 해결됨)

  * 존재하지 않는 헤더파일인데, 사용되고 있고, 게다가 꼭 필요한것 같습니다. 이부분 어떻게하죠?
    * http://code.google.com/p/codecomposer/source/browse/trunk/MIDISoundTest/source/smidlib_pch.cpp#7
    * 이 부분 잡아냈습니다. MakeFile 보시면 알수있습니다.

  * 다른건 다 잡은거 같은데.. safemalloc 이 부분이 난감. plugin\_def.h / plugin.h 함수 포인터로 선언되어 있는데, 도저히 정의 부분 찾을 수가 없음.. 이 부분 알아? by 진모
    * memtool.