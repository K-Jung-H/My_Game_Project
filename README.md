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
- 조명 연산 추가하기
	- 객체를 배열로 CBV로 바인딩하는 방식을 활용하여, 씬에 있는 모든 조명 정보를 필요한 정보만 담은 구조체 배열로 정리하고, 이를 CBV에 복사하여 바인딩하기

- 그림자 추가하기
	- 택스쳐를 리소스 힙 주소영역 기반으로 바인딩 하는 방식을 활용하여, 기존 리소스 힙에 ShadowMap을 저장하는 고정 영역을 할당하고, 해당 영역을 바인딩 하기

- Composite Pass 에서 ShadowMap 뿐만 아니라, 조명 정보 배열을 함께 바인딩 해야 함
- 조명, 그림자 사이에 추가적인 연결 정보가 필요함
	- 해당 그림자는 어떤 조명과 연산 되어야 하는지
	- 조명이 사용하는 ShadowMap의 SRV 인덱스는 몇번인지 등등


진행 상황:
- Light 컴포넌트 추가 // Direction 속성 추가 필요
- Light 컴포넌트 인스팩터 연결 // Direction 추가 필요
- Light Pass 추가 중 // 문제 발생 <-
Composite-Pass 외에 Light-Pass를 별도로 분리


Light-Pass
- Cluster Build Pass // 조명 연산 공간 분리
- Light Assignment Pass // 각 공간에서 처리할 조명 데이터 인덱스 선별
- Lighting Pass // 조명 및 그림자 연산
	-> Lighting Pass 는 Composite-Pass에서 한번에 처리하기 // CS에서 처리하면, 기능적 손해가 많음 // MSAA, 레스터라이저 기능, 블랜딩 기능 등등

사실 Cluster Build, LightAssignment 단계는 하나의 Pass로 처리해도 됨
그러나,
	- GPU 공간 낭비
	- 분리한 공간을 기반으로 후처리에서 활용 가능

-> 3-Pass 단계로 결정


문제 발생
- Light Pass 단계를 추가하고, 리소스 관리 동작 까지 전부 연결하였는데, 문제 발생함
- 일부 텍스쳐가 바인딩이 안됬다는 오류 발생 // 0번 텍스쳐는 바인딩 안되고, 그 외의 텍스쳐는 바인딩이 되고 있다고 함
- Light Pass 추가 전에는 이런 문제가 없었음
- Light Pass 동작을 전부 주석처리하고 실행하였으나, 랜더링되는 오류 화면만 변화가 생기고, 디버깅 툴 오류 문구는 동일함 ###
	-> Light Pass 외에 어딘가 변질 되었음 or 메모리 영역이 오염되었음

가능성
- RootSignature, hlsl 설정 오류 // 새로운 LightPass RootSignature, CS 함수, 전역 hlsli 을 생성, 수정 하면서, 일부 변경한 설정들이 건드렸을 수도 있음
- SRV_Frame에서 SRV_Texture의 Heap 범위 침범 // 리소스 생성시 할당되는 ID를 확인하니, 그런 것 같지는 않음, 그러나 증상이, 0번 인덱스만 UAV 리소스로 침범된 느낌
- UAV or Structured 관리 미흡 // 말 그대로임



-------------------------------------

장기적 목표


* Object 생성 기능 추가하기 // Window 메뉴 박스 활용하기
* Shader 핫리로드 기능 추가하기




