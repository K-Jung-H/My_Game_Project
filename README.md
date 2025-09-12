유니티를 모방하여 게임 엔진 만들기



작업 완료

* 기초(필수)적인 컴포넌트 구조 작성





진행 현황

* 리소스 클래스 정의 // Mesh, Texture, Material
* Assimp를 이용한 FBX 파일 임포트 동작 진행중
* Material Load 동작 작성 완료



문제점

* Texture를 Load 하려면, ID3D12Device 객체와 커멘드 리스트 같은 여러 렌더링 자원이 필요해짐

&nbsp;	->  Load 함수를 호출하는 계층과 렌더링 자원들이 위치한 계층이 다름





