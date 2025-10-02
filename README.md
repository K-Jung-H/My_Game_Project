유니티를 모방하여 게임 엔진 만들기



작업 완료

Scene 클래스를 작성하고 관리하는 구조를 완성

계층구조 Update 로직 작성 및 동작 테스트 완료

자주 쓰는 행렬 연산을 DXMathUtils.h 에 정리

모델 Load 방식 Assimp/FBX\_SDK 혼합 사용 적용



GameTimer 추가

&nbsp;	- FPS 시각화



입력을 저장, 처리하는 InputManager 구현



WindowMessage를 처리하는 플로우 완성

&nbsp;	- WM\_SIZE 화면 크기 변화에 따른 DX 버퍼 리소스 재할당 및 적용 테스트 완료





할 일:

* 계층 구조 Update 로직에, TransformComponent에 상태를 추가하기 // \[상속, 독립, 비활성화]로 업데이트 관리
* Scene에 GameObject를 저장하는 컨테이너 역할 추가하기 
* Scene에 GameObject를 관리하는 기능 추가하기 



* GameTimer 를 통해 얻은 dt를 이용하여, 카메라 움직임 구현하기 // dt 기반 움직임 완성하기
