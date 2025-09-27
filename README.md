유니티를 모방하여 게임 엔진 만들기



작업 완료

기초(필수)적인 컴포넌트 구조 작성

PSO, Shader, RootSignature 관리 구조 완성

Scene 클래스를 작성하고 관리하는 구조를 완성





진행 현황

Renderer 에서 MeshRendererComponent 배열을 받아, DrawCall 기록하는 함수 작성

->  MeshRendererComponent 와 TrnasformComponent 를 함께 저장하는 RenderData 전달용 구조체를 정의하여, 배열을 만들어 전달하도록 수정함



-> Transform 컴포넌트 또는 공용 Transform 전달 버퍼를 추가하여, 바인딩 동작 추가하기

 	-> Renderer에서 매 프레임마다, Transform Pool 담당 버퍼를 만들어서 모든 객체의 Transform을 단일 버퍼로 처리

 		-> Cpu 효율 향상, 랜더링 과정에서의 동작 책임 분리



-> Mesh 에서 저장중인 Material\_ID를 읽어 재질을 바인딩 하게 하는 동작 추가하기

 		->  진행 중 -> 오류 발생







Scene에 GameObject를 저장하는 컨테이너 역할 추가하기 //  진행 우선순위 마지막

Scene에 GameObject를 관리하는 기능 추가하기 //  진행 우선순위 마지막





문제점:

&nbsp;	- 텍스쳐 일부가 잘못 바인딩 되는 현상



원인 분석

&nbsp;	- 1. fbx 파일에서의 uv 좌표계와 directx 의 uv 좌표계  y 축 반전 생략 되었음 

&nbsp;	- 2. fbx 파일을 Unity에서 Load 하여 모델 데이터를 분석해보고, assimp 파일을 그대로 txt로 추출하여 검사하여 비교한 결과, 

&nbsp;																					모델의 일부 데이터가 의도한 것과 다르게 저장 되어 있음

&nbsp;		- 모델의 face를 이루는 부분 Mesh들 중, 마지막 부분 Mesh가 문제 원인으로 밝혀짐.  ->  불필요한 texture을 저장하고 있음

&nbsp;			// 모델의 원본 게임에서 캐릭터의 표정을 변화하기 위한 일부 텍스쳐가 있고, 이를 위한 face의 subMesh 인 듯 함

&nbsp;			// 해당 SubMesh 자체가 불필요하거나, 사용하지 않을 떄에는 비활성화가 되어야 함. 

&nbsp;					// 그렇다고 모델마다 설정이 다를텐데, 사용하지 않을 Mesh를 선별하는 방식은 불가능에 가까움

&nbsp;	

해결 방법

&nbsp;	- 1. Mesh를 Load 하면서 uv의 y 축 반전 적용 -> 해결됨

&nbsp;	- 2.

&nbsp;			-  1. 다른 게임의 fbx 를 Load 시도 해서 이상 없으면, 기존 방법 유지 + 테스트 모델 변경하기 -> 시도 예정

&nbsp;			-  2. 현재 모델을 수정하여 다시 저장

&nbsp;			-  3. 해당 SubMesh 에 사용되는 Texture 파일을 제거 + Load 과정에서 불러오지 못한 Texture을 별도로 표현하고, 해당 Texture을 호출 시도하는 경우,  DrawCall 에서 제외 or 예외(Log 기록) 처리하기



&nbsp;		-> 해결 시도 순서:

&nbsp;				: 3 > 1 > 2

&nbsp;		



