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
    * 부위별 애니메이션 적용을 위한 Avatar Masking (상/하체 분리 등).
    * 레이어별 가중치(Weight) 및 속도 제어.

### Architecture & Tools
* **Resource System:**
    * GUID 기반 에셋 관리, JSON 메타데이터, 비동기 로딩 구조.
    * Transient Resource Pipeline: 애니메이션 전용 파일 로드 시 불필요한 Skeleton/Avatar 에셋을 디스크에 저장하지 않고 메모리상에서만 처리하는 최적화 구현.
* **Editor (ImGui):**
    * 씬 그래프 및 인스펙터.
    * 애니메이션 레이어 실시간 제어 및 블렌딩 상태 시각화.

---

## Currently In Progress & Issues

현재 개발 단계에서 해결 중인 기술적 이슈와 진행 상황.

### 1. Advanced Retargeting (T-Pose Auto-Correction)
서로 다른 구조를 가진 이기종 모델 간의 애니메이션 공유를 위한 자동 보정 시스템을 구현 중.

* **문제점 (Current Issue):**
    * 표준 T-Pose 모델(Anya)의 애니메이션을 A-Pose 모델(CP_Series)에 적용 시, 팔다리가 기괴하게 꺾이거나 회전축이 어긋나는 현상 발생.
    * 특히 3ds Max Biped의 **Twist Bone**이 포함된 모델에서 증상이 심각함.

* **원인 분석:**
    * **Bind Pose 불일치:** 소스(0도, T-Pose)와 타겟(45도, A-Pose)의 기본 각도가 달라, 절대값 회전을 적용하면 각도가 중첩됨.
    * **로컬 축(Axis) 상이:** Twist Bone으로 인해 자식 뼈의 로컬 좌표계가 회전되어 있어, 동일한 회전값을 적용해도 다른 방향으로 굽혀짐.

* **해결 방안 (Solution Strategy):**
    * **로딩 시점:** 모델의 뼈가 가리키는 방향 벡터를 분석하여, 강제로 T-Pose를 만들기 위한 **보정 회전값(Correction Quaternion)**을 계산하여 `Avatar`에 저장.
    * **런타임:** 애니메이션 적용 전, 보정값을 먼저 곱해 가상의 T-Pose 상태로 만든 후 애니메이션 델타를 적용.
    * **수식:** `R_Final = R_Bind * R_Correction * R_AnimDelta`

### 2. Root Motion
* 애니메이션의 루트 본(Hips) 이동 데이터를 추출하여 실제 GameObject의 월드 좌표 이동으로 변환하는 로직 구현 예정.

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