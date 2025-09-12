유니티를 모방하여 게임 엔진 만들기



작업 완료

* 기초(필수)적인 컴포넌트 구조 작성





진행 현황

* 리소스 클래스 정의 // Mesh, Texture, Material
* Assimp를 이용한 FBX 파일 임포트 동작 진행중, FBX Load 테스트 진행 완료
* 서술자 힙을 관리하는 클래스 Descriptor\_Manager 정의 // ID 할당 + FreeList 기반





문제점

* FBX load 시도 결과, Mesh, Material 에는 load 동작 이상이 없으나, texture load 시도 시, 문제 발생

&nbsp;	-> texture 파일의 주소가 .fbx 파일과 같은 위치가 아닌, 프로젝트 파일 위치에서 탐색을 시도함

&nbsp;	->  파일의 주소를 받는 방식을 변경해야 함

&nbsp;		->  (호출시) fbx 파일 위치 + fbx 파일 내에 저장된 texture 위치 로 하여, 파일 주소를 재정의 해야 함



