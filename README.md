유니티를 모방하여 게임 엔진 만들기



작업 완료

* 기초(필수)적인 컴포넌트 구조 작성
* PSO, Shader, RootSignature 관리 구조 완성
* Scene 클래스를 작성하고 관리하는 구조를 완성





진행 현황

* 리소스, 컴포넌트를 바인드 할 수 있는 GameObject 클래스 구현하기 V

 	->  Object 클래스의 세부 내용 작성

 	->     재질은 Mesh 바인딩할 때, Mesh에 필요한 재질 ID로 함께 바인딩하기

 	->     카메라 컴포넌트 정의 및 프래임 동작에 바인딩 추가 완료



* 기본 Shader, RootSignature 바인딩 동작 추가하기 <- 테스트 진행 도중 문제 발생

 	-> Scene에서 활성화된 Mesh\_Renderer 컴포넌트 묶음을 받아 랜더링 호출하는 기능 DX12\_Renderer 에 추가 중



* Scene에 GameObject를 저장하는 컨테이너 역할 추가하기
* Scene에 GameObject를 관리하는 기능 추가하기



PSO 문제 해결

* PSO 생성 함수에서 셰이더 컴파일 Blob 객체들을 지역 변수로 생성하여, PSO 생성 전에 소멸 하는 문제엿음

&nbsp;		->  문제 해결 완료

&nbsp;		->  셰이더 컴파일 방식 전환할 필요 없음

