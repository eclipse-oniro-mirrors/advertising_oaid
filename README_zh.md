# 广告标识服务部件

## 简介

开放匿名设备标识符（Open Anonymous Device Identifier, OAID，以下简称OAID）：是一种非永久性设备标识符，基于开放匿名设备标识符，可在保护用户个人数据隐私安全的前提下，向用户提供个性化广告，同时三方监测平台也可以向广告主提供转化归因分析。


## 目录

```
/domains/advertising/oaid  # 广告标识服务部件业务代码
├── interfaces                         # 接口代码
├── profile                            # 服务配置文件
├── services                           # 服务代码
├── test                               # 测试用例
├── LICENSE                            # 证书文件
└── bundle.json                        # 编译文件
```

## 使用说明

### 获取OAID

可以使用此仓库内提供的接口获取OAID。以下步骤描述了如何使用接口获取OAID。

1. 申请广告跟踪权限

   在模块的module.json5文件中，申请[ohos.permission.APP_TRACKING_CONSENT](https://docs.openharmony.cn/pages/v4.0/zh-cn/application-dev/security/permission-list.md/)权限。

   ```javascript
   {
     "module": {
       "requestPermissions": [
         {
           "name": "ohos.permission.APP_TRACKING_CONSENT" // 申请广告跟踪权限
         }
       ]
     }
   }
   ```

2. 在应用启动时触发动态授权弹框，向用户请求授权。

   ```javascript
  import abilityAccessCtrl from '@ohos.abilityAccessCtrl';
  import { BusinessError } from '@ohos.base';
  import hilog from '@ohos.hilog';
   
  private requestOAIDTrackingConsentPermissions(context: common.Context): void {
    // 进入页面时触发动态授权弹框，向用户请求授权广告跟踪权限
    const atManager: abilityAccessCtrl.AtManager = abilityAccessCtrl.createAtManager();
      try {
        atManager.requestPermissionsFromUser(context, ["ohos.permission.APP_TRACKING_CONSENT"]).then((data) => {
          if (data.authResults[0] == 0) {
            hilog.info(0x0000, 'testTag', '%{public}s', 'request permission success');
          } else {
            hilog.info(0x0000, 'testTag', '%{public}s', `user rejected`);
          }
       }).catch((err) => {
         const e: BusinessError = err as BusinessError;
         hilog.error(0x0000, 'testTag', '%{public}s', `request permission failed, error message: ${e.message}`);
       })
    } catch(err) {
      const e: BusinessError = err as BusinessError;
      hilog.error(0x0000, 'testTag', '%{public}s', `catch err->${JSON.stringify(e)}`);
    }
  }
   ```

3. 获取OAID信息

- 通过Callback回调函数获取OAID

  ```javascript
  import identifier from '@ohos.identifier.oaid';
  import hilog from '@ohos.hilog'; 
  import { BusinessError } from '@ohos.base';

  try {
    identifier.getOAID((err, data) => {
      if (err.code) {
        hilog.info(0x0000, 'testTag', '%{public}s', `getOAID failed, message: ${err.message}`);
    } else {
      const oaid: string = data;
      hilog.info(0x0000, 'testTag', '%{public}s', `getOAID by callback success`);
    }
    });
  } catch (err) {
    const e: BusinessError = err as BusinessError;
    hilog.error(0x0000, 'testTag', 'get oaid catch error: %{public}d %{public}s', e.code, e.message);
  }
  ```

- 通过Promise异步获取OAID

  ```javascript
  import identifier from '@ohos.identifier.oaid';
  import hilog from '@ohos.hilog'; 
  import { BusinessError } from '@ohos.base';
  
  try {  
    identifier.getOAID().then((data) => {
      const oaid: string = data;
      hilog.info(0x0000, 'testTag', '%{public}s', `get oaid by callback success`);
    }).catch((err) => {
      hilog.info(0x0000, 'testTag', '%{public}s', `get oaid failed, message: ${(err as BusinessError).message}`);
    })
  } catch (err) {
    const e: BusinessError = err as BusinessError;
    hilog.error(0x0000, 'testTag', 'get oaid catch error: %{public}d %{public}s', e.code, e.message);
  }
  ```

### 重置OAID

可以使用此仓库内提供的接口重置OAID。以下步骤描述了如何使用接口重置OAID。该接口为系统接口。

```javascript
  import identifier from '@ohos.identifier.oaid';
  import hilog from '@ohos.hilog'; 
  import { BusinessError } from '@ohos.base';
 
  try {
    identifier.resetOAID();
  } catch (err) {
    const e: BusinessError = err as BusinessError;
    hilog.error(0x0000, 'testTag', 'reset oaid catch error: %{public}d %{public}s', e.code, e.message);
  }
```

## 相关仓