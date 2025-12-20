# GameEngine_DX12

DirectX 12 기반의 자체 제작 3D 게임 엔진 프로젝트.
Entity-Component-System (ECS) 아키텍처 기반.

## Implemented Features

### Graphics & Rendering
* **DirectX 12 Core:** Low-level 리소스 관리, Descriptor Heap 추상화, PSO 캐싱.
* **Lighting Pipeline:**
    * PBR (Physically Based Rendering) Shader.
    * Directional, Point, Spot Lights.
    * Cascaded Shadow Maps (CSM) & PCF Filtering.
* **Model Support:**
    * Static / Skinned Mesh 렌더링.
    * FBX SDK & Assimp 라이브러리를 활용한 하이브리드 로더.

### Animation System
* **GPU Skinning:** Compute Shader를 활용한 고속 버텍스 스키닝.
* **Animation Controller:**
    * 상태 기반 애니메이션 재생 (Loop, Once, Ping-Pong).
    * Cross-fading (Smooth Blending) 지원.
* **Animation Layer:**
    * 다중 레이어(Base, Overlay) 지원.
    * 레이어별 가중치(Weight) 및 속도 제어.
* **Advanced Retargeting (Auto-Correction):**
    * 이기종 모델(T-Pose ↔ A-Pose) 간 애니메이션 공유 시 발생하는 **골격 늘어짐(Bone Stretching)** 현상 해결.
    * **구현 방식:** 로딩 시점 Bind Pose와 타겟 포즈 간의 차이를 분석, 보정 회전값(Correction Quaternion)을 계산하여 런타임에 자동 적용.
    * *Note:* 일부 모델에서 발생하는 Candy Wrapper Effect(비틀림 시 부피 감소)문제는 모델 토폴로지 및 웨이트 구조의 문제로 판단하여 현재 엔진의 해결 범위에서 제외함.

### Architecture & Tools
* **Resource System:**
    * GUID 기반 에셋 관리, JSON 메타데이터, 비동기 로딩 구조.
    * Transient Resource Pipeline: 애니메이션 전용 파일 로드 시 불필요한 Skeleton/Avatar 에셋을 디스크에 저장하지 않고 메모리상에서만 처리하는 최적화 구현.
* **Editor (ImGui):**
    * 씬 그래프 및 인스펙터.
    * **Node-based Graph Editor:**
        * 노드/링크 편집, Zoom & Pan, 미니맵(Minimap) 지원.
        * 다중 선택(Quad Selection) 및 곡선(Bezier)/직선 링크 시각화.
    * 애니메이션 레이어 실시간 제어 및 블렌딩 상태 시각화.

### GUI & Resource File Importer

* **Resource Management & Pipeline:**
    * **Assimp Integration:** FBX/OBJ 등 외부 모델 포맷 지원, Mesh/Skeleton/Animation 데이터 추출 및 변환 파이프라인.
    * **Robust GUID Generation:** Assimp 노드 이름 중복("Scene", "Mesh" 등)으로 인한 충돌 방지를 위해 `Name + Index` 조합의 고유 식별자 생성 알고리즘 적용.
    * **Recursive Meta-data System:**
        * JSON 기반 `.meta` 파일을 생성하여 원본 에셋과 엔진 내부 에셋 간의 영속성(Persistence) 보장.
        * `sub_resources` 배열 순회를 통해 단일 파일 내 포함된 다수의 Mesh, Material, Animation Clip 리소스를 개별 GUID로 식별 및 로드.

* **Inspector & Editor Interface (ImGui):**
    * **Component-based Inspection:**
        * `SkinnedMeshRenderer`, `AnimationController` 등 컴포넌트 타입별 전용 UI 제공.
        * **Multi-SubMesh Support:** 복수의 서브 메시(Sub-mesh)로 구성된 모델의 슬롯별 재질(Material) 할당 및 시각화 인터페이스.
    * **Material Workflow:**
        * PBR 속성(Albedo, Roughness, Metallic) 실시간 제어.
        * 텍스처 슬롯에 대한 **Drag & Drop** 지원, Hover 시 썸네일 미리보기(Texture Preview) 기능.
    * **UI Architecture Stability:**
        * **ID Stack Management:** 동일 컴포넌트 다중 부착 또는 동일 이름의 위젯 렌더링 시 발생하는 ID 충돌(Conflict) 방지를 위해, 컴포넌트 포인터 기반(`PushID(ptr)`)의 엄격한 ID Scope 관리 구현.
        * **Resource Picker:** 에셋 타입별 자동 필터링, 검색 기능이 포함된 콤보박스 및 Drag & Drop 페이로드 시스템 연동.
    * **Docking System:** Viewport, Hierarchy, Inspector, Resource Browser 등 패널의 자유로운 배치 및 도킹 지원.

### Terrain System
* **QuadTree-based LOD (Level of Detail):**
    * **Space Partitioning:** 쿼드트리(QuadTree)를 활용한 공간 분할 및 거리 기반 LOD 레벨 결정.
    * **Optimized Culling:** 카메라 좌표를 로컬 공간(Local Space)으로 변환하여 연산 부하를 최소화한 절두체 컬링(Frustum Culling) 구현.
* **GPU-Driven Tessellation:**
    * **Hardware Tessellation:** Hull & Domain Shader를 사용하여 런타임에 지형 지오메트리 동적 생성.
    * **Instanced Rendering:** 1x1 단위 패치(Unit Quad Patch)와 인스턴싱을 활용하여 드로우 콜(Draw Call) 획기적 감소.
* **Dual Height Data Management:**
    * **GPU:** Displacement Mapping을 위한 고속 텍스처 샘플링.
    * **CPU:** 물리 충돌 및 로직 처리를 위해 정밀도가 보장된 `float` 기반 높이 데이터 베이킹(Baking).

---

## Currently In Progress & Issues

현재 개발 단계 진행 상황.

Terrain 렌더링 구현 중
- 타일 인스턴싱 구현 완료
- LOD 기반 동적 테셀레이션 구현 완료
-> 변위 매핑 구현 <- 진행중

DS 에서 전달받은 하이트맵을 샘플링하여, 변위 매핑을 진행해야 함

변위매핑이 안일어나고 있는 상태

예상 문제 원인:
	- 텍스쳐에 정보가 저장되는 방식이 잘못된 경우 <- 유력함, 디버깅 중
	- 샘플링 실패
	- heightScale 배율 조정 실수


---

## Getting Started

1.  **Prerequisites:**
    * **CMake** (Latest version recommended).
    * Visual Studio 2022 (C++20 Standard).
    * Windows 10/11 SDK.
    * DirectX 12 compatible GPU.

2.  **Build (via CMake):**
    * Create a folder named `Build` in the project root directory.
    * Generate project files using CMake targeting the `Build` folder.
        * *Command Line:* `cmake -S . -B Build`
    * Open the generated `.sln` file located inside the `Build` folder.
    * Select `Debug` or `Release` configuration and Build Solution (Ctrl+Shift+B).

3.  **Run:**
    * Run (F5).

## License

This project is licensed under the MIT License.