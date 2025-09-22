유니티를 모방하여 게임 엔진 만들기



작업 완료

기초(필수)적인 컴포넌트 구조 작성

PSO, Shader, RootSignature 관리 구조 완성

Scene 클래스를 작성하고 관리하는 구조를 완성





진행 현황

Renderer 에서 MeshRendererComponent 배열을 받아, DrawCall 기록하는 함수 작성

MRT 상태 관리 문제 발생

G-Buffer ->  Composite-Pass ->  BackBuffer 흐름 작성 필요





Scene에 GameObject를 저장하는 컨테이너 역할 추가하기 //  진행 우선순위 마지막

Scene에 GameObject를 관리하는 기능 추가하기 //  진행 우선순위 마지막





문제점:

&nbsp;		MRT 상태 관리 문제

&nbsp;		- 현재 ClearGBuffer() 안에서 MRT를 RENDER\_TARGET → 다시 COMMON으로 돌려버림.

&nbsp;		- 그래서 드로우 시점에는 MRT가 더 이상 RTV가 아니고, 대신 백버퍼 RTV만 연결된 상태임.

&nbsp;		- 그런데 PSO는 여전히 MRT 포맷을 기대하고 있음 → 포맷 불일치 에러 발생.

&nbsp;		->  MRT는 지오메트리 패스가 끝날 때까지 RENDER\_TARGET 상태로 유지해야 함.



&nbsp;	G-Buffer → BackBuffer 전달 과정 없음

&nbsp;		- MRT(G-Buffer)에 geometry 데이터를 썼다 해도, 그걸 읽어서 최종 화면(BackBuffer)에 써주는 컴포지트 패스가 없음.

&nbsp;		- 즉 “MRT에 기록은 되는데 화면에는 반영되지 않는” 상태.

&nbsp;		->  지오메트리 패스 후 MRT를 SRV로 전환하고, 백버퍼를 RTV로 바인딩해서 풀스크린 패스로 MRT 결과를 샘플링

&nbsp;		==  백버퍼에 출력해야 함.







