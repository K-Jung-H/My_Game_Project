유니티를 모방하여 게임 엔진 만들기



작업 완료

* 기초(필수)적인 컴포넌트 구조 작성





진행 현황

* PSO, Shader, RootSignature 관리 구조 완성
* Scene 클래스를 작성하고 관리하는 구조를 완성해야 함





문제점

* Scene의 Class 타입을 Resource를 상속 받게 해야 하는가?





---

Scene이 Resource를 상속 받는 경우



장점:

엔진의 리소스 관리 시스템(ResourceManager) 안에서 다른 리소스(Mesh, Texture, Material 등)와 같은 방식으로 관리 가능

ResourceID, Path, Alias 같은 공통 관리 기능을 바로 재사용 가능

씬 전환이 자원 로딩 구조와 일관성 있음

->  씬 관리가 쉽다.



단점:

Scene은 단순한 리소스가 아님 → GameObject, Camera, Light 같은 런타임 오브젝트 컨테이너 역할까지 담당

Scene의 생명주기는 Texture/Mesh와 달리 훨씬 복잡함 (씬 활성화, 업데이트, 비활성화 등)

Resource 인터페이스에 맞추려다 Scene 설계가 꼬일 수 있음





---

Scene을 독립적으로 구현하는 경우



장점:

Scene은 게임의 "월드 컨테이너"로, 자원와 성격이 다르므로 더 자연스러움

SceneManager 같은 별도 관리자가 존재하면 구조가 명확해짐 (ex: 현재 씬, 로딩 대기 씬, 이전 씬 등)

Scene은 오브젝트/카메라/라이트 같은 런타임 엔티티 위주로 관리 가능

-> 게임 월드 관리가 쉽다.



단점:

리소스 시스템과 완전히 따로 놀게 됨 → 예: Scene을 에디터에서 ResourceID로 참조하는 기능이 필요하면 별도 관리 코드가 필요

씬 로드/저장 로직을 직접 구현해야 함





===

씬의 기본적인 구조(Build, Update, Physics Update, Particle Update)의 기반을 먼저 완성하고,



씬을 리소스처럼 ID 로 관리하는 Scene\_Manager를 구현하기.



Scene\_Manager는 Scene에서 ResourceManager에 원할하게 접근할 수 있어야 하므로,

GameEngine의 맴버 변수(같은 계층)으로 적용하기.





