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



문제점:
런타임 시 생성한 Save 파일만 Load 시 객체들이 제대로 그려지고 있음 == 외부에서 세팅한 Save 파일이 제대로 반영 안됨

원인:
- 생성한 meta 파일을 프로젝트 실행시 사용하지 않고, 새로 GUID를 할당하여 사용하고 있음 
-> 프로그램 실행 시, Asset 폴더 내부에 있는 모든 meta 파일을 읽어, 파일 경로 기반 GUID map을 만들고, Load 시, 리소스에 대한 중복 ID 생성 시도를 방지해야 함

- fbx의 meta 파일이 제대로 생성 안됨
-> fbx 파일 내부에 mesh 마다 GUID 할당, 연결된 Mat 파일의 GUID, Texture 파일의 GUID를 fbx 처음 Load 시 저장해야 함

- Scene Load 동작에서 컴포넌트 할당 과정에, ComponentRegistry가 오히려 불편하여 이를 제거하였으나, 리소스 Load, Registry 과정에서도 같은 문제가 있음
-> ResourceManager, ResourceRegistry의 기능을 통합한 ResourceSystem을 새로 정의하여, 코드를 정리해야 함


문제 해결 진행중
- 불편했던 리소스 관리 체계를 ResourceSystem으로 통일
	- 실행 시, Assets 폴더의 모든 Meta 파일 데이터 저장
	- Meta 생성, 중복, 저장 관리
	- 리소스 Load 체계 단순화

-> 외부에서 세팅한 Save 파일이 제대로 반영 안되는 문제 해결을 위한 준비 완료
--> GUID 기반 SAVE, LOAD 동작 다시 검사 하기


문제 해결:
- GUID 관리 미흡
	- FBX와 같은 복합 리소스 파일에 대한 meta 파일이 제대로 생성 안됨
	- 각 부분 메시들에 대한 독립적인 GUID 생성 실패 // 경로 기반, 동일한 메시 이름으로 인해, 동일한 GUID 가 생성되었음

-> FBX 파일의 경우, 부분 메시들마다 중복 이름을 갖지 못하게 유일함을 보장함 -> 중복 GUID 해결
-> GUID를 무작위로 생성하는 방식 이외에, 경로+이름 문자열 기반으로 암호화 하는 방식 추가 -> 다른 환경에서도 동일한 GUID 생성 보장



-------------------------------------

장기적 목표


* Object 생성 기능 추가하기 // Window 메뉴 박스 활용하기
* Shader 핫리로드 기능 추가하기




