유니티를 모방하여 게임 엔진 만들기



작업 완료

기초(필수)적인 컴포넌트 구조 작성

PSO, Shader, RootSignature 관리 구조 완성

Scene 클래스를 작성하고 관리하는 구조를 완성





진행 현황

Renderer 에서 MeshRendererComponent 배열을 받아, DrawCall 기록하는 함수 작성

MRT 상태 관리 기능 ->  랜더링 단계 분리 ->  랜더링 단계 구현 중

&nbsp;	- 랜더링 단계 분리 (Geometry / Composite / PostProcess / Blit / imgui )

&nbsp;	- 각 단계에 사용해야 하는 루트 시그니쳐 + 셰이더 구현 중

&nbsp;		- 루트 시그니쳐는 전환이 최소화 되어야 함 

&nbsp;			- Geometry 용 Default 타입

&nbsp;			- 그 외 단계에 사용되는 PostFX 타입

&nbsp;		



Scene에 GameObject를 저장하는 컨테이너 역할 추가하기 //  진행 우선순위 마지막

Scene에 GameObject를 관리하는 기능 추가하기 //  진행 우선순위 마지막


Shader 파일 생성 완료 및 연결 확인 완료
각 Shader별로 PSO 설정 확인 필요 // 오류 발생 중





