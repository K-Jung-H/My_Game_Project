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





할 일:

* Scene Save/Load 기능을 추가하기

* 계층 구조 Update 로직에, TransformComponent에 상태를 추가하기 // \[상속, 독립, 비활성화]로 업데이트 관리
* Window 메뉴를 추가하고, 해당 메뉴와 상호작용하는 Engine 에서의 동작 추가 중
* 애니메이터 컴포넌트 추가하기 // 기존 Skeleton 구조를 활용하기


진행 상황:
- 씬 데이터를 저장하고 불러오는 기능을 추가하려 하는 중
	- 저장/불러오기 진입 과정까지는 연결 됨
	- Editor Mode에서는 JSON 방식으로 읽기/저장을 하는 구조로 작성

- Scene Data를 저장하는 JSON 파일을 생성하는 함수를 구현하기
- Scene Data를 저장한 JSON 파일을 해석하여 Scene을 생성하는 함수를 구현하기

JSON은 Object 단위로 관리할 것


프로젝트에 모든 GUID를 저장하고 있는 json 파일을 만들어, 프로젝트 리소스를 관리하기
// 구현 및 적용이 완료되면, Window 메뉴 창에 Scene Save/Load 기능을 추가하기


-------------------------------------

장기적 목표


* Object 생성 기능 추가하기 // Window 메뉴 박스 활용하기
* Shader 핫리로드 기능 추가하기




