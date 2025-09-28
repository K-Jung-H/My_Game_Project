유니티를 모방하여 게임 엔진 만들기



작업 완료

Scene 클래스를 작성하고 관리하는 구조를 완성

계층구조 Update 로직 작성 및 동작 테스트 완료

자주 쓰는 행렬 연산을 DXMathUtils.h 에 정리





할 일:

* 계층 구조 Update 로직에, TransformComponent에 상태를 추가하기 // \[상속, 독립, 비활성화]로 업데이트 관리
* Scene에 GameObject를 저장하는 컨테이너 역할 추가하기 //  진행 우선순위 마지막
* Scene에 GameObject를 관리하는 기능 추가하기 //  진행 우선순위 마지막
* Timer 추가하고, Fps 시각화 하기



진행 현황:

* FBX 불러오는 과정에서 Assimp 호환 버전 문제 발생 // 문제점





문제점:

* FBX 불러오는 과정에서 Assimp 호환 버전 문제 발생 

&nbsp;	- Assimp가 구형 FBX 형식을 지원하지 않음, 그러나 그 외의 다양한 포맷을 지원함

&nbsp;		- Assimp를 통한 다양한 포맷 처리의 일관성 유지 가능, 그러나, FBX가 거의 표준 포맷임



해결 방법

* FBX SDK를 프로젝트에 추가하여,  FBX 파일 로드 방식을 담당하게 한다.

&nbsp;		- Assimp에서 우선적으로 Load 를 실행하고, 문제가 발생 시, FBX 포맷인 경우, FBX SDK 를 이용한 방법으로 전환하기

->  시도 예정



