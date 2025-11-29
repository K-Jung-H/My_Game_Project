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
### 1. Root Motion
* 애니메이션의 루트 본(Hips) 이동 데이터를 추출하여 실제 GameObject의 월드 좌표 이동으로 변환하는 로직 구현 예정.
- 사용하던 애니메이션들에서 문제 발생
	- Mixamo로 추출하는 애니메이션의 경우, 루트 모션이 없음
	- Unity로 애니메이션을 추출 시도
		- FBX_EXPORTER: 모델과 애니메이션 파일의 리타게팅 구조가 FBX 파잃에 저장 안됨 
		- Clip을 Bake 하는 Script 작성: 루트 모션이 추출 안됨. 
	
	- Blender 를 사용하여 Mixamo에서 루트 모션을 생성하여 적용
		- Blender 숙련도 부족

-> 루트 모션에 계획된 것보다 너무 많은 시간 할애함. 
-> 애니메이션은 구조 정리 후, 다른 것부터 처리하기.

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