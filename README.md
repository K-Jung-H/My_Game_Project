유니티를 모방하여 게임 엔진 만들기
────────────────────────────────────────────

작업 완료
────────────────────────────────────────────
Scene 클래스를 작성하고 관리하는 구조를 완성
계층구조 Update 로직 작성 및 동작 테스트 완료
자주 쓰는 행렬 연산을 DXMathUtils.h 에 정리
모델 Load 방식 Assimp/FBX_SDK 혼합 사용 적용
GameTimer 추가
입력을 저장, 처리하는 InputManager 구현
WindowMessage를 처리하는 플로우 완성
물리적 움직임(속력/가속도/충돌) 담당 RigidbodyComponent 구현 완료
입력에 따른 dt 기반 카메라 움직임 추가
Scene에 GameObject를 저장하는 컨테이너 역할 추가
무조건 ObjectManager를 통해 Object를 생성하도록 권한 설정 // ID 기반 관리 및 동시 생성 및 접근 대응
조명 Component 및 연산 추가 - 다중 조명 테스트 완료
- Cluster Light 연산을 이용한 조명 연산 최적화


할 일
────────────────────────────────────────────
1)
그림자 구조 최적화
+ Spot 조명에 AABB 컬링 추가하기
  // 컬링이 되기는 하나 프러스텀 영역이 조명 영역보다 넓게 설정되는 중
  // 투영행렬과 Reverse Z 방식에서 충돌하는 것으로 추정

디버깅
────────────────────────────────────────────
- Release 모드 실행 시, 카메라 움직임에 따라 일부 영역 깜빡임 발생
  = Directional / Point Light에서 사각형 영역의 그림자 깜빡임

분석 내용:
  - 그림자 매핑 연산 자체가 불안정하게 동작 중
  - 카메라가 멀어질 때 또는 위로 이동할 때 주로 발생
  - FrameResource, OnResize 등 리소스 갱신 문제는 아님
  - 프레임레이트가 높을수록(1500fps 이상) 깜빡임 빈도 증가
  - Debug 모드에서는 문제 미발생 (프레임 저하로 인한 완화)

초기 추측:
  - ShadowMatrix 업데이트 타이밍 불일치
  - Depth Bias, Slope Bias 설정 불안정
  - Reverse Z 구조와 ShadowPass 설정 충돌 가능성

────────────────────────────────────────────
[문제 원인 및 해결 과정 정리]
────────────────────────────────────────────
문제 현상:
  - 카메라 고정 시에도 특정 위치에서 그림자 패턴 지속
  - 포인트/디렉셔널 라이트 이동 시, 검은 사각형 형태의 패턴 발생
  - bias 값을 키우면 문제 발생 빈도 감소

원인 발견:
  - ShadowMap 렌더링 시 CullMode 설정이 문제였음
  - 기존: D3D12_CULL_MODE_BACK
  - 변경: D3D12_CULL_MODE_FRONT
  - Reverse-Z 투영 구조에서 CullMode가 뒤집혀야 함을 확인

상세 설명:
  - Reverse-Z 방식에서는 깊이 비교 방향이 GREATER_EQUAL로 반전됨.
  - 기존 BACK 컬링은 “빛을 맞는 앞면”만 남겨 잘못된 깊이값 기록.
  - FRONT 컬링은 “빛을 막는 뒷면(back face)”만 남겨 올바른 깊이 저장.
  - 이로 인해 Bias 유무와 관계없이 섀도우맵의 일관성이 확보됨.
  - 깜빡임의 본질은 부동소수 오차가 아닌 “잘못된 면(front face)”의 깊이 기록이었다.

적용 결과:
  - 그림자 깜빡임 완전 해소
  - 높은 FPS 환경에서도 안정적인 Shadow Rendering 유지
  - Bias 설정(±5000) 변화에도 안정적으로 동작

최종 PSO 설정:
  Rasterizer:
    desc.CullMode = D3D12_CULL_MODE_FRONT;
    desc.DepthBias = +100;
    desc.SlopeScaledDepthBias = +1.5f;
  DepthFunc:
    D3D12_COMPARISON_FUNC_GREATER_EQUAL; // Reverse-Z 기준

요약 결론:
  → Reverse-Z 투영에서는 CullMode를 FRONT로 바꿔야 함.
  → 기존 BACK 모드 사용 시, 잘못된 면의 깊이값이 기록되어 깜빡임 발생.
  → Bias로는 해결 불가능한 구조적 문제였음.
  → FRONT 컬링으로 근본적 해결.


────────────────────────────────────────────
장기적 목표
────────────────────────────────────────────
* Object 생성 기능 추가하기 (Window 메뉴 박스 활용)
* Shader 핫리로드 기능 추가하기
────────────────────────────────────────────