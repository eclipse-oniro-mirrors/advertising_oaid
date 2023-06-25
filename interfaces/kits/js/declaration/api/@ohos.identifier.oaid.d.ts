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

import type { UIAbilityContext } from './application/UIAbilityContext';
import type { AsyncCallback } from './basic';

/**
 * Obtains Open Advertising Identifier(OAID) information.
 * @namespace identifier
 * @syscap SystemCapability.Cloud.OAID
 * @since 10
 */
declare namespace identifier {
  /**
   * Get the open advertising identifier id.
   *
   * @permission ohos.permission.APP_TRACKING_CONSENT
   * @param abilityContext The context of an ability.
   * @param callback The callback to get the open advertising identifier id.
   * @throws {BusinessError} 17300001 - System internal error.
   * @syscap SystemCapability.Cloud.OAID
   * @since 10
   */
  function getAdsIdentifierInfo(abilityContext: UIAbilityContext, callback: AsyncCallback<string>): void;

  /**
   * Get the open advertising identifier id.
   *
   * @permission ohos.permission.APP_TRACKING_CONSENT
   * @param abilityContext The context of an ability.
   * @return Returns the open advertising identifier id.
   * @throws {BusinessError} 17300001 - System internal error.
   * @syscap SystemCapability.Cloud.OAID
   * @since 10
   */
  function getAdsIdentifierInfo(abilityContext: UIAbilityContext): Promise<string>;

  /**
   * Reset the open advertising identifier id.
   *
   * @permission ohos.permission.APP_TRACKING_CONSENT
   * @param abilityContext The context of an ability.
   * @throws {BusinessError} 17300001 - System internal error.
   * @syscap SystemCapability.Cloud.OAID
   * @systemapi
   * @since 10
   */
  function resetAdsIdentifier(abilityContext: UIAbilityContext): void;
}
export default identifier;

