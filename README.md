# OAID Service Component

## Introduction

The Open Anonymous Device Identifier (OAID) service facilitates personalized ad placement based on OAIDs, each of which is a non-permanent device identifier. The service provides personalized ads for users while protecting their personal data privacy. It can also interact with third-party tracking platforms to provide conversion attribution analysis for advertisers.

## Directory Structure

```
/domains/advertising/oaid # Service code of the OAID service component
├── interfaces                         # API code
├── profile                            # Service configuration profile
├── services                           # Service code
├── test                               # Test cases
├── LICENSE                            # License file
└── bundle.json                        # Build file
```

## How to Use

### Obtaining an OAID

You can use the APIs provided in this repository to obtain OAIDs.  

1. Request the ad tracking permission.

   Configure the [ohos.permission.APP_TRACKING_CONSENT](https://docs.openharmony.cn/pages/v4.0/zh-cn/application-dev/security/permission-list.md/) permission in the **module.json5** file of the module.

   ```javascript
   {
     "module": {
       "requestPermissions": [
         {
           "name": "ohos.permission.APP_TRACKING_CONSENT" // Request the ad tracking permission.
         }
       ]
     }
   }
   ```

2. Request authorization from the user by displaying a dialog box when the application is started.

   ```javascript
   import abilityAccessCtrl from '@ohos.abilityAccessCtrl';
   
   private requestOAIDTrackingConsentPermissions(context: any): void {
     // Display a dialog box when the page is displayed to request the user to grant the ad tracking permission.
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

3. Obtain an OAID.

- Obtain an OAID through the callback function.

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

- Obtain an OAID through the promise.

  ```javascript
  import identifier from '@ohos.identifier.oaid';
  
  private getOaidByPromise() {
    try {
      // Obtain an OAID.
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

### Resetting the OAID

You can use the API provided in this repository to reset OAIDs.  The API is a system API.

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

## Repositories Involved
