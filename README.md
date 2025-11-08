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

────────────────────────────────────

할 일

────────────────────────────────────

1)  Skinning Animation 구현 (CS Pre-Skinning 방식)
    - `Mesh` 기본 구조 리팩토링: 현재의 'Separate Vertex Buffers'(속성 분리) 방식에서 'Interleaved Vertex Buffers'(교차 저장) 방식으로 변경.
    - 정점 스트림 분리: 정점 버퍼를 'Hot/Cold' 스트림으로 분리.
        - Hot (Dynamic): 스키닝에 종속되는 `Position`, `Normal`, `Tangent` (Interleaved).
        - Cold (Static): 정적인 `UV`, `Color` (Interleaved).
    - `VertexFlags` 추가: `Mesh` 클래스에 `VertexFlags` (비트마스크)를 추가하여, 해당 메시가 어떤 정점 속성(Normal, Tangent 유무 등)을 가졌는지 명시하도록 수정.

────────────────────────────────────

진행 상황

────────────────────────────────────
		
1. 구조적 문제 (The Problem)

- 상태-데이터 혼동: `SkinnedMesh`는 여러 인스턴스가 공유하는 '리소스'임에도, `mFrameSkinnedBuffers`라는 '인스턴스별 상태' 버퍼를 소유하려 함.
- Race Condition (경합 상태): 씬에 동일 메시("Orc_Head")를 공유하는 "Orc1"과 "Orc2"가 있을 때, 
	`SkinningPass`가 "Orc1"(달리기)의 결과를 쓴 동일한 프레임 버퍼(`mFrameSkinnedBuffers[1]`)에 "Orc2"(공격)의 결과가 덮어쓰여, 모든 인스턴스가 마지막으로 쓰인 애니메이션 포즈로 렌더링되는 오류.
- 컨트롤러 부재: 씬에 배치된 동일 `Model`의 여러 인스턴스(예: "Orc1", "Orc2")가 각기 다른 애니메이션("달리기", "공격")을 재생하도록 제어할 중앙 "컨트롤러"의 부재.
- 그룹핑 정보 누락: `Renderer`의 `mVisibleItems` 목록은 평면적이어서, "Orc1의 머리"와 "Orc1의 팔"이 동일한 애니메이션 상태(동일한 본 행렬 버퍼)를 공유해야 한다는 그룹핑 정보가 누락됨.

2. 해결 방안: 신규 아키텍처 설계 (The Solution)

데이터(공유 리소스)와 상태(고유 인스턴스)를 명확히 분리.

신규/수정 리소스 정의 (Data)

- `Skeleton` (신규 리소스):
    - `Game_Resource`를 상속.
    - 기존 `Model`이 소유하던 `Skeleton` 구조체를 독립 리소스 파일(예: ".skel")로 분리.
    - 뼈대 계층, 뼈 이름, 바인드 포즈 행렬 등 원본 청사진 데이터를 저장.
    - `Model` 리소스 및 여러 `SkinnedMesh`가 이 `Skeleton` 리소스를 공유(참조).

- `AnimationClip` (신규 리소스):
    - `Game_Resource`를 상속.
    - 뼈 이름을 키(key)로 하는 시간대별 키프레임(P, Q, S) 데이터만 순수하게 저장. (예: "run.anim")
    - `Model` 및 `Skeleton`과 완전히 분리되어 재사용성 극대화.

- `SkinnedMesh` (수정된 리소스):
    - `Game_Resource`를 상속.
    - `FrameSkinBuffer mFrameSkinnedBuffers` 멤버 및 `CreatePreSkinnedOutputBuffer` 함수를 리소스 클래스에서 제거.
    - 순수한 정적 데이터 컨테이너 역할:
        - 원본 정점 (Hot/Cold Interleaved VBs).
        - 스킨 가중치/인덱스 (`mSkinData` Buffer).
    - 로드 시점(`Skinning_Skeleton_Bones`)에 특정 `Skeleton` 리소스의 인덱스를 기준으로 베이킹(baking)됨.

신규 컴포넌트 정의 (State)

- `AnimationController` (신규 컴포넌트):
    - 애니메이션 `Object` 계층의 루트(Root)에 부착 (예: "Orc_Instance_Root").
    - 역할 (State): "인형 조종사"로서 `mCurrentClip`, `mCurrentTime`, 블렌딩 가중치 등 모든 동적 상태를 소유.
    - 참조 (Data): `Skeleton` 리소스(청사진)와 `AnimationClip` 리소스(재생 목록)를 참조.
    - 작업 (CPU Update): `Update()` 시 참조된 데이터를 바탕으로 최종 본(bone) 행렬을 계산.
    - GPU 소유권 (State): 계산된 최종 행렬들을 업로드할 자신만의 고유 GPU Structured Buffer (SRV)를 소유하고 매 프레임 업데이트.

- `SkinnedMeshRendererComponent` (신규 컴포넌트):
    - `MeshRendererComponent`를 상속.
    - 실제 `SkinnedMesh`가 렌더링되는 리프(Leaf) 노드에 부착 (예: "Orc_Arm_L").
    - GPU 소유권 (State): `FrameSkinBuffer mFrameSkinnedBuffers[Frame_Render_Buffer_Count];`를 멤버로 직접 소유. (Pre-Skinning의 최종 출력 버퍼).
    - 캐시 포인터 (State): `AnimationController* mCachedAnimController;` 멤버를 소유.
    - 초기화 (Awake/Start):
        1. (탐색) `Object` 계층을 루트까지 탐색, `AnimationController`를 찾아 `mCachedAnimController`에 캐싱. (매 프레임 탐색 문제 해결)
        2. (생성) `CreatePreSkinnedOutputBuffer()` 로직을 스스로 실행하여, 
		`SkinnedMesh` 리소스의 정점 정보를 바탕으로 자신만의 고유한 `mFrameSkinnedBuffers` (UAV/VBV)를 생성. (Race Condition 문제 해결)

3. 신규 데이터 흐름 (Data Flow)

1. CPU Update Loop (매 프레임):
    - "Orc1"의 `AnimationController`가 "run" 행렬을 계산, 자신의 Bone-SRV-1에 업로드.
    - "Orc2"의 `AnimationController`가 "attack" 행렬을 계산, 자신의 Bone-SRV-2에 업로드.

2. 렌더 목록 생성 (매 프레임):
    - `UpdateObjectCBs`가 "Orc1_Arm_L"의 `SkinnedMeshRendererComponent`를 발견.
    - `DrawItem`에 다음 정보를 복사:
        - `SkinnedMesh` 리소스 포인터 (데이터).
        - `mCachedAnimController` 포인터 (상태: Bone-SRV-1).
        - `SkinnedMeshRendererComponent` 포인터 (상태: Output-UAV/VBV-1).

3. GPU `SkinningPass` (매 프레임):
    - "Orc1_Arm_L"의 `DrawItem`을 처리:
    - INPUT 1 (Data): `SkinnedMesh` 리소스의 원본 정점 (Hot VB SRV).
    - INPUT 2 (Data): `SkinnedMesh` 리소스의 스킨 가중치 (`mSkinData` SRV).
    - INPUT 3 (State): `mCachedAnimController`의 Bone-SRV-1 (달리기 행렬).
    - OUTPUT (State): `SkinnedMeshRendererComponent`의 Output-UAV-1 (`mFrameSkinnedBuffers[mFrameIndex].uavSlot`).
    - `Dispatch` 실행. (이후 "Orc2" 처리 시, Bone-SRV-2와 Output-UAV-2가 사용됨).

4. GPU `GeometryPass` (매 프레임):
    - `SkinnedMesh::Bind` 로직이 `DrawItem`을 통해 `SkinnedMeshRendererComponent`에 접근하도록 수정됨.
    - "Orc1_Arm_L"을 그릴 때, `SkinnedMeshRendererComponent`가 소유한 Output-VBV-1 (`mFrameSkinnedBuffers[mFrameIndex].vbv`)을 바인딩.
    - "Orc2_Arm_L"을 그릴 때, `SkinnedMeshRendererComponent`가 소유한 Output-VBV-2를 바인딩.




장기적 목표

────────────────────────────────────
- Object 생성 기능 추가하기 (Window 메뉴 박스 활용)
- Shader 핫리로드 기능 추가하기
