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

import type { AsyncCallback } from './basic';

/**
 * OAID(Open Advertising Identifier), a non-permanent device identifier.
 *
 * @since 10
 * @syscap SystemCapability.Cloud.OAID
 * @permission N/A
 * @import oaid from '@ohos.identifier.oaid';
 */
declare namespace oaid {
  /**
   * Get OAID(Open Advertising Identifier).
   *
   * @since 10
   * @syscap SystemCapability.Cloud.OAID
   * @param { AsyncCallback<string> } callback - Indicates the callback to get the OAID(Open Advertising Identifier).
   * @throws {BusinessError} 401 - Invalid input parameter.
   * @throws {BusinessError} 17200001 - System internal error.
   * @returns { void | Promise<string> } no callback return Promise otherwise return void.
   */
  function getOAID(callback: AsyncCallback<string>): void;
  function getOAID(): Promise<string>;

  /**
   * Get setting of disable personalized ads.
   *
   * @since 10
   * @syscap SystemCapability.Cloud.OAID
   * @param { AsyncCallback<boolean> } callback - Indicates the callback to get the setting of disable personalized ads.
   * @throws {BusinessError} 401 - Invalid input parameter.
   * @throws {BusinessError} 17200001 - System internal error.
   * @returns { void | Promise<boolean> } no callback return Promise otherwise return void.
   */
  function isLimitAdTrackingEnabled(callback: AsyncCallback<boolean>): void;
  function isLimitAdTrackingEnabled(): Promise<boolean>;
}

export default oaid;
