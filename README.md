유니티를 모방하여 게임 엔진 만들기

────────────────────────────────────

작업 완료

────────────────────────────────────

- Scene 클래스를 작성하고 관리하는 구조를 완성
- 계층구조 Update 로직 작성 및 동작 테스트 완료
- 자주 쓰는 행렬 연산을 DXMathUtils.h 에 정리
- 모델 Load 방식 Assimp/FBX_SDK 혼합 사용 적용
- GameTimer 추가
- 입력을 저장, 처리하는 InputManager 구현
- WindowMessage를 처리하는 플로우 완성
- 물리적 움직임(속력/가속도/충돌) 담당 RigidbodyComponent 구현 완료
- 입력에 따른 dt 기반 카메라 움직임 추가
- Scene에 GameObject를 저장하는 컨테이너 역할 추가
- 무조건 ObjectManager를 통해 Object를 생성하도록 권한 설정 (ID 기반 관리 및 동시 생성 및 접근 대응)
- 조명 Component 및 연산 추가 - 다중 조명 테스트 완료
    - Cluster Light 연산을 이용한 조명 연산 최적화
    - 그림자 구현 완료
- Skinning 구현 완료

────────────────────────────────────

할 일

────────────────────────────────────


씬(Scene) 저장/불러오기 직렬화
- `Object::FromJSON`에 `SkinnedMeshRendererComponent` 및 `AnimationControllerComponent` 타입 추가.
- 각 컴포넌트에 `ToJSON` / `FromJSON` 함수를 구현하여, `meshId`, `Model_Avatar` GUID, `AnimationClip` GUID 등을 저장/로드.

────────────────────────────────────

진행 상황

────────────────────────────────────

- Avatar, Skeleton 의 리소스 생성 및 리타게팅 제대로 동작함
	- 각 리소스 파일을 확인해서 Bone 매핑 결과를 확인 하였음 // Humanoid 에 필요한 기본적인 구조는 모두 매핑되는걸 확인

문제 상황:
- 모델을 불러올 때, T-Pose가 아닌 모델인 경우, 애니메이션이 왜곡되는 오류가 발생하는 중
	- 불러올 때 T-Pose로 변환하던가, 기본 Bind-Pose에 대한 정리를 하고 넘어가야 함

────────────────────────────────────

장기적 목표

────────────────────────────────────

- Object 생성 기능 추가하기 (Window 메뉴 박스 활용)
- Shader 핫리로드 기능 추가하기