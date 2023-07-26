/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import type { AsyncCallback } from './@ohos.base';

/**
 * This module provides the capability to manage OAID.
 *
 * @namespace identifier
 * @syscap SystemCapability.Cloud.OAID
 * @since 10
 */
declare namespace identifier {
  /**
   * Get the OAID.
   * @permission ohos.permission.APP_TRACKING_CONSENT
   * @param { AsyncCallback<string> } callback - The callback to get the OAID.
   * @throws { BusinessError } 17300001 - System internal error.
   * @syscap SystemCapability.Cloud.OAID
   * @since 10
   */
  function getOAID(callback: AsyncCallback<string>): void;

  /**
   * Get the OAID.
   * @permission ohos.permission.APP_TRACKING_CONSENT
   * @return { Promise<string> } Returns the OAID.
   * @throws { BusinessError } 17300001 - System internal error.
   * @syscap SystemCapability.Cloud.OAID
   * @since 10
   */
  function getOAID(): Promise<string>;

  /**
   * Reset the OAID.
   * @throws { BusinessError } 17300001 - System internal error.
   * @syscap SystemCapability.Cloud.OAID
   * @systemapi
   * @since 10
   */
  function resetOAID(): void;
}
export default identifier;

