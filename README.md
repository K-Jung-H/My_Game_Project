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

---

## Currently In Progress & Issues

현재 개발 단계 진행 상황.
### 1. ResourceSystem GUI 구현
* ResourceSystem 에서 관리하는 리소스들의 컨트롤러 역할
	- 속성 정보 확인 // 리소스 타입에 따른 디테일 추가하기 <- 구현 및 테스트 완료
	- 씬 뷰와 상호작용 
		- 리스스 뷰에서 씬 뷰로 드래그 & 드랍 및 변경 적용 <- 구현 및 테스트 완료
		- 오브젝트(+자식) 생성, 제거 기능 추가 <- 구현 및 테스트 완료
		- 오브젝트 이름 변경 기능 추가 <- 구현 및 테스트 완료
	- 외부 파일 Import <- 구현 및 테스트 완료


### 2. Scene Save/Load 업데이트
* 추가된 컴포넌트, 리소스들을 SceneData로 저장하는 기능 추가
	- Scene Save 동작 <- 테스트 완료
	- Scene Load 동작 <- 문제(1)

문제(1)
- Animation Controller Component Load 시, SkinnedMesh 가 변화하지 않고, Bind-Pose로 대기함
	- AnimationController의 Skeleton 데이터 초기화 or SkinnedMesh Component 의 Skinning 버퍼 초기화 동작이 생략되어 있을 가능성 있음
		- 원인: 
			- 일부 컴포넌트는 생성시 버퍼 생성과 같은 초기 준비 작업이 필요함.
			- 기존 Scene Build 단계에서는 초기 준비 작업까지 미리 처리하지만, Scene Load 시, Build와 동작이 다름

		- 해결 방법: 
			- 컴포넌트에 각 컴포넌트 별 초기화 동작을 호출하는 WakeUp 라는 공용 함수를 추가
			- Build 마무리 및 Load 마무리 단계에 Scene에 있는 모든 RootObject를 순회하며, 보유한 컴포넌트들의 WakeUp을 호출하는 로직 추가
			-> 해결


- 런타임에 Import 한 모델을 씬에 배치하고 Save 후, 프로그램 재실행 후 Scene Load 에서 Mesh 들이 연결 안되고, MeshComponent에는 None으로 선택되어 있음
	- 프로그램 실행 시 Asset 폴더 내부에 있는 meta 및 guid 가 등록된 파일들을 모두 Load 하는 동작 필요
	- 런타임 시 import 한 meta 파일 및 guid 관리 방식 검토 필요
		- 문제 해결 진행중:
			- SkinnedMeshComponent에 대한 처리 개선
			- GUID 기반 Load 및 Path 관리 강화
			- 뮤텍스를 사용하는 컴포넌트 타입 분리

		- 새로운 문제점:
			- 현재 Scene Load 시, AnimationController Component에 Avatar, Skeleton 이 Load 및 Setter 호출이 확인됬지만, Scene_Load 완료 시, Empty로 전환되고 있음

- 문제 해결 진행 중:
		

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