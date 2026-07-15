# Youllee's Preload Manager

Unreal Engine에서 자주 사용하는 데이터 에셋을 Editor World와 Game World에 미리 로드해두기 위한 플러그인입니다.

프로젝트 설정에서 'DataAsset', 'DataTable', 'DataRegistry'를 등록하면, 지정한 월드 수명주기에 맞춰 에셋을 동기 로드하고 강한 참조로 유지합니다.

게임 시스템이 데이터를 요청하기 전에 필요한 에셋을 미리 준비해두고 싶을 때 사용할 수 있습니다.

![YL Preload Manager Settings](Docs/YL_Preload_Manager_Setting_1.1.png)

## v1.1 Update

- Preload Context 세분화 : Editor, **Game** -> Editor, **Server**, **Client**
- 중복 Load 시, 스킵 로직 추가
- SyncLoad, AsyncLoad 선택 가능
- Preload 성공/실패 로그 추가
- Setting Widget 개선 : Widget 자동 줄바꿈 추가
- 불필요한 config 파일 제거
  
## 주요 기능

- 'DataAsset', 'DataTable', 'DataRegistry' Preload
- Preload 시점 설정 (Editor World / Game World)

## 설치

'YLPreloadManager' 폴더를 프로젝트의 'Plugins' 폴더 아래에 복사합니다.

```text
YourProject/
  Plugins/
    YLPreloadManager/
      YLPreloadManager.uplugin
      Source/
      Config/
      Resources/
```

그 후 Unreal Editor를 다시 실행하고, 필요하면 플러그인을 활성화한 뒤 프로젝트를 다시 빌드합니다.

## 설정 위치

프로젝트 설정에서 아래 경로로 이동합니다.

```text
Project Settings > Game > Youllee's Preload Manager
```

설정 화면에는 세 종류의 프리로드 목록이 있습니다.

- **Data Assets**: 'UDataAsset' 기반 에셋을 등록합니다.
- **Data Tables**: 'UDataTable' 에셋을 등록합니다.
- **Data Registries**: 'UDataRegistry' 에셋을 등록합니다.

각 항목에는 다음 옵션이 있습니다.

- **Preload on Editor**: 에디터 월드에서 프리로드합니다.
- **Preload on Game**: 게임 월드에서 프리로드합니다.

## 에셋 수명 (v1.1)

이 플러그인은 로드한 에셋을 'TStrongObjectPtr'로 보관합니다.
따라서 프리로드 목록에 들어간 에셋은 해당 수명주기가 끝날 때까지 GC 대상이 되지 않습니다.

- Preload on Editor : true인 경우, Engine 초기화 이후 로드되고, Module 셧다운 시 언로드 됩니다.
- Preload on Server : true인 경우, Server World 초기화 이후 로드되고, Server World 클린업 시 언로드 됩니다.
- Preload on Client : true인 경우, Client World 초기화 이후 로드되고, Client World 클린업 시 언로드 됩니다.

## 로드 방식 (v1.1)

이 플러그인은 동기/비동기 로드 중 한가지를 선택할 수 있습니다.

- bUseAsyncLoad : true인 경우, 비동기 방식으로 에셋을 로드합니다.

## 갱신 시점

1. Editor에서 Preload Manager Setting이 변경된 경우
2. Editor에서 Preload된 Asset이 저장된 경우

기존에 로드된 에셋을 리셋하고, 다시 로드합니다.

## 주의사항

- 프리로드는 동기 로드로 처리됩니다. 에셋의 용량이 크거나 지나치게 많은 에셋을 등록하면, 프리로드 시 비용이 커질 수 있습니다.
- 이 플러그인은 에셋을 미리 로드하고 참조를 유지하는 용도이며, 모든 데이터 초기화 순서를 자동으로 보장하지 않습니다.

## Current Status

- Developed with Unreal Engine 5.7
- MIT License

## License

MIT License. 자세한 내용은 [LICENSE](LICENSE)를 참고하세요.
