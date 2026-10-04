# TokenVault

> 一個以 Qt 6 / C++17 開發的本機 Token 管理工具，使用 OpenSSL 保護敏感資料。

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-6.5%2B-green.svg)](https://www.qt.io/)
[![OpenSSL](https://img.shields.io/badge/OpenSSL-1.1.1%2B-lightgrey.svg)](https://www.openssl.org/)

TokenVault 可在本機安全地管理 Token，支援密碼、鑰匙檔與 TOTP 驗證，並提供群組、搜尋、期限管理與自動鎖定功能。

## 主要功能

- 使用密碼、鑰匙檔及 TOTP 保護 Vault。
- 支援密碼 + 鑰匙檔 + TOTP 的多因素驗證規則。
- 以 AES-256-GCM 加密 Vault 索引與 Token 資料。
- 使用 PBKDF2-HMAC-SHA512 衍生金鑰。
- 使用 RSA-4096 OAEP 保護每個 Token 的加密金鑰。
- Token 群組管理與關鍵字搜尋。
- Token 有效期限、作廢、更新與刪除。
- 期限提醒，3 天內到期的 Token 會特別標示。
- 顯示 Token 後自動隱藏；複製後自動清除剪貼簿。
- 閒置自動鎖定，也可使用 `Ctrl+L` 立即鎖定。
- 支援淺色／深色外觀與可攜式資料目錄設定。

## 使用方式

1. 第一次啟動時設定至少一種金鑰材料：密碼或鑰匙檔。
2. 若同時使用密碼與鑰匙檔，必須再啟用 TOTP。
3. 使用驗證器 App 掃描 QR Code，例如 Google Authenticator、Microsoft Authenticator、Aegis 或 1Password。
4. 在主畫面建立群組與 Token。
5. 點選 Token 可查看詳細資料；顯示 Token、作廢、更新、刪除及變更期限前需要重新驗證。
6. 若忘記密碼，可從解鎖畫面選擇「忘記密碼」。此操作會永久刪除所有群組與 Token，且無法復原。

## 建置需求

- CMake 3.21 或更新版本
- C++17 相容編譯器
- Qt 6.5 或更新版本（專案主要以 Qt 6.8 開發）
- OpenSSL 1.1.1 或更新版本（建議使用 OpenSSL 3.x）

## 建置與測試

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

若系統安裝的 Qt 版本低於專案預設版本，可透過 `TV_QT_MIN` 指定最低版本：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTV_QT_MIN=6.4
```

## 設定

| 環境變數 | 說明 |
|---|---|
| `TOKENVAULT_DIR` | 指定 Vault 資料目錄，適合攜帶式使用或測試 |
| `TOKENVAULT_THEME` | 強制指定外觀：`dark` 或 `light` |

預設資料會儲存在作業系統的 AppData／應用程式資料目錄中。

## 專案結構

```text
src/                 核心程式與 Qt UI
src/crypto.*         加密與雜湊功能
src/totp.*           TOTP 驗證
src/vault.*          Vault 儲存、加密與 Token 管理
third_party/         第三方元件（包含 qrcodegen）
tests/               單元測試
packaging/           Linux 安裝與桌面檔案
DESIGN.md            加密架構、設計理由與限制
```

## 安全性說明

- TOTP 是介面層的額外驗證，不會直接成為檔案加密金鑰的一部分。
- Token 的「作廢」是本機狀態標記，不會呼叫外部服務撤銷 Token。
- Qt 的字串元件無法保證密碼與 Token 從記憶體完全清除。
- 檔案刪除前會嘗試覆寫，但 SSD 與寫時複製檔案系統不保證實體資料完全清除。
- 若攻擊者同時取得 Vault 檔案與密碼／鑰匙檔，仍可能解密資料；請使用足夠強度的密碼並妥善保管鑰匙檔。

完整的加密架構與設計取捨請參閱 [`DESIGN.md`](DESIGN.md)。

## 第三方授權

第三方元件與授權資訊請參閱 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

## 授權

目前尚未在儲存庫中指定正式授權條款。若要重新散布或修改本專案，請先確認專案作者的授權意願。
