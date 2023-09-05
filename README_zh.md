# 广告标识服务部件

### 简介

开放匿名设备标识符（Open Anonymous Device Identifier, OAID，以下简称OAID）：是一种非永久性设备标识符，基于开放匿名设备标识符，可在保护用户个人数据隐私安全的前提下，向用户提供个性化广告，同时三方监测平台也可以向广告主提供转化归因分析。


### 目录

```
/domains/cloud/oaid  # 广告标识服务部件业务代码
├── interfaces                         # 接口代码
├── profile                            # 服务配置文件
├── services                           # 服务代码
├── test                               # 测试用例
├── LICENSE                            # 证书文件
└── bundle.json                        # 编译文件
```

### 说明

#### 使用说明

##### 获取OAID

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
   
   private requestOAIDTrackingConsentPermissions(context: any): void {
     // 进入页面时触发动态授权弹框，向用户请求授权广告跟踪权限
     const atManager = abilityAccessCtrl.createAtManager();
     try {
       atManager.requestPermissionsFromUser(context, ["ohos.permission.APP_TRACKING_CONSENT"]).then((data) => {
         if (data.authResults[0] == 0) {
           console.info(`request permission success`);
         } else {
           console.info(`user rejected`);
         }
       }).catch((err) => {
         console.error(`request permission failed, error message: ${err.message}`);
       })
     } catch(err) {
       console.error(`catch err->${JSON.stringify(err)}`);
     }
   }
   ```

3. 获取OAID信息

- 通过Callback回调函数获取OAID

  ```javascript
  import identifier from '@ohos.identifier.oaid';
  
  private getOaidByCallback() {
    try {
      identifier.getOAID((err, data) => {
        if (err.code) {
          console.info(`getAdsIdentifierInfo failed, message: ${err.message}`);
  	  } else {
  		const oaid = data;
  		console.error(`getOaidFromOaidSaAPi by callback success`);
  	  }
  	});
    } catch (err) {
      console.error(`catch err->${JSON.stringify(err)}`);
    }
  }
  ```

- 通过Promise异步获取OAID

  ```javascript
  import identifier from '@ohos.identifier.oaid';
  
  private getOaidByPromise() {
    try {
      // 获取OAID信息
      identifier.getOAID().then((data) => {
        const oaid = data;
        console.info(`getAdsIdentifierInfo by promise success`);
      }).catch((err) => {
        console.error(`getAdsIdentifierInfo failed, message: ${err.message}`);
      })
    } catch (err) {
      console.error(`catch err->${JSON.stringify(err)}`);
    }
  }
  ```

##### 重置OAID

可以使用此仓库内提供的接口重置OAID。以下步骤描述了如何使用接口重置OAID。该接口为系统接口。

```javascript
import identifier from '@ohos.identifier.oaid';
 
private resetOaid() {
  try {
    identifier.resetOAID();
  } catch (err) {
    console.error(`reset oaid catch error: ${err.code} ${err.message}`);
  }
}
```

### 相关仓