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
    - `SkinnedMesh` 수정: 리팩토링된 `Mesh` 구조를 `SkinnedMesh`가 상속받고, `mSkinDataBuffer` (뼈 가중치/인덱스)를 추가로 관리하도록 수정. 또한, 원본 'Hot' 버퍼 외에 CS 스키닝 결과물을 저장할 별도의 'Dynamic Output Buffer'(UAV+VBV)를 멤버로 추가.
    - CS Pre-Skinning Pass 구현: Interleaved 'Hot' 버퍼를 입력(SRV)/출력(UAV)으로 받는 HLSL 컴퓨트 셰이더 및 C++ `Dispatch` 로직 구현. (CS 셰이더 로직은 계산 결과를 이 'Dynamic Output Buffer'(UAV)에 저장해야 함).
    - 렌더링 파이프라인 수정: `Renderer`의 `Bind` 로직(혹은 `SkinnedMesh::Bind` virtual)이 렌더링 시 원본 'Hot' 버퍼가 아닌 CS가 기록한 'Dynamic Output Buffer'를 VBV로 바인딩하도록 수정.
    - ModelLoader 수정: `ModelLoader_Assimp` 및 `ModelLoader_FBX`가 `VertexFlags`를 설정하고, 'Hot/Cold' Interleaved 버퍼를 생성하도록 로직 변경.
    - PSO 변형(Permutation) 적용: `Renderer`가 메시의 `VertexFlags`를 읽어, 해당 정점 구성에 맞는 올바른 CS/VS 셰이더(PSO)를 선택하도록 `PSO_Manager` 로직 확장.

────────────────────────────────────

장기적 목표

────────────────────────────────────
- Object 생성 기능 추가하기 (Window 메뉴 박스 활용)
- Shader 핫리로드 기능 추가하기
