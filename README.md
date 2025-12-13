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

구현 및 테스트 완료
- Terrain System 구현
	- TerrainResource: 지형맵 리소스
		- TerrainHeightField: HeightMap CPU 데이터
		- TerrainQuadTree: 트리 기반 공간 관리
			- TerrainNode: 트리 노드

	- TerrainComponent: TerrainResource 관리
	- TerrainPatchMesh: 패치 단위 인스턴싱 - 테셀레이션

-> raw 파일 Load 기능 테스트
-> CPU 저장 구조 테스트 결과 확인


진행 예정
- Terrain 관련 셰이더 코드 작성
- **Terrain Mesh Rendering (GeometryTerrainPass) 구현**
    - **UploadBuffer & DynamicBufferAllocator 시스템 도입**
        - **구현 필요성:**
            - Terrain System은 카메라 거리에 따라 LOD가 변하므로, 매 프레임 전송해야 할 인스턴스 데이터(`TerrainInstanceData`)의 개수가 가변적임.
            - 기존의 정적 버퍼 방식이나 매 프레임 버퍼 생성(`CreateCommittedResource`) 방식은 메모리 할당 오버헤드로 인해 성능 저하 유발.
        - **핵심 기능 구현 요소:**
            - **`UploadBuffer`:**
                - `ID3D12Resource` (Upload Heap)를 래핑하여 CPU-GPU 간 데이터 전송 통로 확보.
                - **Linear Allocation:** 커서(Offset) 기반의 선형 할당을 통해 메모리 파편화 방지 및 고속 할당 지원.
                - **Memory Mapping:** CPU 데이터 복사 주소와 GPU 바인딩 주소(Virtual Address)의 쌍(Pair)을 반환.
            - **`DynamicBufferAllocator`:**
                - 다수의 `UploadBuffer`를 **페이지(Page)** 단위로 관리하는 상위 할당자.
                - **Page Growing Strategy:** 현재 페이지의 용량 부족 시, 자동으로 새로운 페이지를 생성 및 연결하여 메모리 오버플로우 방지.
                - **Frame Lifecycle Management:** 프레임 종료 시 할당된 모든 페이지의 커서를 초기화(`Reset`)하여 재사용(Triple Buffering 지원).
        - **적용 시 장점:**
            - **Zero Allocation Overhead:** 미리 할당된 거대한 메모리 풀을 사용하여 런타임 할당 비용 제거.
            - **Direct Memory Access:** `memcpy`를 통해 CPU의 `DrawList` 데이터를 GPU 메모리로 즉시 복사.
            - **Batching & Scalability:** 지형 데이터뿐만 아니라 파티클, UI 등 엔진 내 모든 동적 데이터 처리에 통합 적용 가능한 유연한 구조 확보.


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