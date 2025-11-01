유니티를 모방하여 게임 엔진 만들기



작업 완료

Scene 클래스를 작성하고 관리하는 구조를 완성

계층구조 Update 로직 작성 및 동작 테스트 완료

자주 쓰는 행렬 연산을 DXMathUtils.h 에 정리

모델 Load 방식 Assimp/FBX\_SDK 혼합 사용 적용

GameTimer 추가

입력을 저장, 처리하는 InputManager 구현

WindowMessage를 처리하는 플로우 완성

물리적 움직임(속력/가속도/충돌) 담당 RigidbodyComponent 구현 완료

입력에 따른 dt 기반 카메라 움직임 추가

Scene에 GameObject를 저장하는 컨테이너 역할 추가

무조건 ObjectManager를 통해 Object를 생성하도록 권한 설정 // ID 기반 관리 및 동시 생성 및 접근 대응

조명 Component 및 연산 추가 - 다중 조명 테스트 완료
- Cluster Light 연산을 이용한 조명 연산 최적화



할 일:

1):
---------------------
그림자 구조 최적화
+ Point/Spot 조명에 AABB 컬링 추가하기
+ 카메라 프러스텀 컬링 추가하기

디버깅
- Release 모드 실행 시, 카메라 움직임에 따라 일부 영역 깜빡임 발생 중 // Directional/Point Light에서 사각형 영역의 그림자 발생 및 깜빡임
 


-------------------------------------

진행 상황:


장기적 목표


* Object 생성 기능 추가하기 // Window 메뉴 박스 활용하기
* Shader 핫리로드 기능 추가하기




