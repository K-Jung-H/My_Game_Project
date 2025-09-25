유니티를 모방하여 게임 엔진 만들기



작업 완료

기초(필수)적인 컴포넌트 구조 작성

PSO, Shader, RootSignature 관리 구조 완성

Scene 클래스를 작성하고 관리하는 구조를 완성





진행 현황

Renderer 에서 MeshRendererComponent 배열을 받아, DrawCall 기록하는 함수 작성

->  MeshRendererComponent 와 TrnasformComponent 를 함께 저장하는 RenderData 전달용 구조체를 정의하여, 배열을 만들어 전달하도록 수정함



-> Transform 컴포넌트 또는 공용 Transform 전달 버퍼를 추가하여, 바인딩 동작 추가하기

-> Mesh 에서 저장중인 Material\_ID를 읽어 재질을 바인딩 하게 하는 동작 추가하기







Scene에 GameObject를 저장하는 컨테이너 역할 추가하기 //  진행 우선순위 마지막

Scene에 GameObject를 관리하는 기능 추가하기 //  진행 우선순위 마지막





