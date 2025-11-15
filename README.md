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

1. 현재 상태

- `AnimationControllerComponent`와 `SkinnedMeshRendererComponent` 아키텍처 구현 완료.
- 데이터와 상태를 분리하여, 씬(Scene)의 각 객체(Object)가 고유한 애니메이션 상태를 가질 수 있음.
- `SkinningPass` (Compute Shader)가 `GeometryPass` 이전에 스키닝을 수행하고, 렌더링 시 이 결과물(VBV)을 사용함.
- `ModelLoader`가 `Skeleton` 리소스를 위상 정렬(Topological Sort)하여 계층 구조 붕괴 문제 해결.
- FBX SDK 로더의 `inverseBind` 행렬 계산 로직을 수정하여 Assimp 로더와 동일한 결과를 보장함.
- 현재 한계: `AnimationControllerComponent`가 하드코딩된 애니메이션(예: `sin(time)`)을 사용 중이며, 리타겟팅(Retargeting)이 불가능함.

2. 향후 구현 로드맵 (Retargeting System)

하드코딩된 테스트 환경을 데이터 기반의 리타겟팅 애니메이션 시스템으로 업그레이드.

1단계: Engine_AvatarDefinition ("기본 뼈 사전") 정의
- `Game_Resource`가 아닌, 엔진 내부의 정적(static) 데이터로 `Engine_AvatarDefinition` 추상 클래스 정의.
- `Human_AvatarDefinition`, `Animal_AvatarDefinition` 등 타입별 파생 클래스에 "Head", "Spine" 같은 "키(Key)" 뼈 목록을 하드코딩.

2단계: AvatarDefinitionManager (전역 관리자) 구현
- `Human_AvatarDefinition` 등 "기본 뼈 사전"의 전역 인스턴스를 소유하고 관리하는 싱글톤 매니저 클래스 구현.
- `DefinitionType` (enum)을 키로 `Engine_AvatarDefinition` 인스턴스에 접근하는 `GetDefinition()` 함수 제공.

3단계: Model_Avatar ("매핑 테이블") 리소스 정의
- `Game_Resource`를 상속받는 `Model_Avatar` 리소스 클래스 정의.
- 내부에 `DefinitionType mDefinitionType` (예: `Humanoid`)과 `std::map<string, string> mBoneMap` ("키" -> "실제 뼈 이름")을 멤버로 가짐.
- `AutoMap(std::shared_ptr<Skeleton> skeleton)` 멤버 함수를 구현. (2단계의 매니저를 참조하여 자동 추측 로직 수행)

4단계: ModelLoader 수정 (`.avatar` 파일 자동 생성)
- `ModelLoader_FBX` / `ModelLoader_Assimp`가 `Skeleton` 로드 시, `Model_Avatar`를 생성하도록 수정.
- `SetDefinitionType(DefinitionType::Humanoid)` 및 `AutoMap(skeletonRes)`을 호출.
- `ResourceSystem`에 등록하여 `.avatar` 파일로 저장.
- `Model` 리소스가 이 `.avatar` 파일의 ID/GUID를 참조하도록 `Model` 클래스 수정.

5단계: AnimationClip 리소스 정의
- `Game_Resource`를 상속받는 `AnimationClip` 리소스 클래스 정의.
- 내부에 `TKey<T>` (키프레임), `AnimationTrack` (뼈 기반 Pos/Rot/Sca) 구조체 정의.
- `AnimationClip` 멤버:
  - `DefinitionType mAvatarDefinitionType;` (이 클립이 사용하는 "기본 뼈 사전"의 타입)
  - `std::map<std::string, AnimationTrack> mTracks;` (Key: "추상 뼈 이름" (예: "Spine"))

6단계: ModelLoader 수정 (`.anim` 파일 추출)
- `ModelLoader`가 씬의 애니메이션 데이터를 `AnimationClip` 리소스로 추출하도록 수정.
- 4단계에서 생성된 `Model_Avatar`의 `mBoneMap`을 역참조하여, 모델의 실제 뼈 이름을 "추상 뼈 이름"(Key)으로 번역하여 `mTracks`에 저장.
- `ResourceSystem`에 등록하여 `.anim` 파일과 `.meta` 파일을 생성.

7단계: AnimationControllerComponent 업그레이드
- `AnimationControllerComponent`가 `std::shared_ptr<Model_Avatar> mAvatar`와 `std::shared_ptr<AnimationClip> mCurrentClip`을 참조하도록 수정.
- `SkinnedMeshRendererComponent::Initialize`가 `Model`로부터 `Model_Avatar`를 찾아 컨트롤러에 설정하도록 수정.
- `EvaluateAnimation`에서 하드코딩된 로직을 제거.
- `mSkeleton` -> `mAvatar` (번역) -> `mCurrentClip` (샘플링) 순서로 `localTransforms`를 계산하도록 최종 로직 구현.

8단계: 씬(Scene) 저장/불러오기 직렬화
- `Object::FromJSON`에 `SkinnedMeshRendererComponent` 및 `AnimationControllerComponent` 타입 추가.
- 각 컴포넌트에 `ToJSON` / `FromJSON` 함수를 구현하여, `meshId`, `Model_Avatar` GUID, `AnimationClip` GUID 등을 저장/로드.

────────────────────────────────────

진행 상황

────────────────────────────────────

- 애니메이션 관리 구조 및 구현 로드맵 설계 완료
- 5~6단계 일부까지 진행 완료 // ModelLoader`에서 애니메이션을 AnimationClip 리소스로 추출 까지 진행

다음 할 일:	
- AnimationClip을 리소스로 저장하기, 불러오기 동작 추가하기 // .anim 파일로 저장 및 .meta 파일 생성
- 테스트용 AnimationClip 데이터를 생성하기 // 현재 사용하는 모델 파일에 Animation이 없음. 외부에서 fbx 파일에 Clip을 추가하여 준비해야 함
- Model_Avatar::AutoMap 함수 구현해서 클립과 대상 Model_Avatar가 같은 Key를 보유하는지 동작 테스트 하기


────────────────────────────────────

장기적 목표

────────────────────────────────────

- Object 생성 기능 추가하기 (Window 메뉴 박스 활용)
- Shader 핫리로드 기능 추가하기