# TokenVault — Token 管理工具

Qt 6.8 / C++17 / OpenSSL。Windows、macOS、Linux（x86_64 與 aarch64）。

## 使用方式
1. 第一次啟動：設定「密碼」、「鑰匙檔」或兩者並用（鑰匙檔以內容的 SHA-512 雜湊作為鎖）。
2. 之後啟動只需解鎖一次。主畫面一開始只有左下角的 **＋**：建立群組 / 建立 Token。
3. 點 Token 進入詳細頁。**顯示 Token、作廢、更新、刪除、變更有效期限**都要重新驗證；改名稱與說明不用。
4. 顯示的 Token 30 秒後自動隱藏；複製後 30 秒自動清除剪貼簿；閒置 5 分鐘（可調）自動鎖定。
5. Token 有效顯示綠色勾，過期或作廢顯示紅色叉；啟動與每分鐘會檢查並提醒過期（及 3 天內到期）。

資料夾：預設在系統的 AppData 位置，可用環境變數 `TOKENVAULT_DIR` 指定（方便攜帶或測試）。

## 建置
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```
需要 Qt ≥ 6.5（Linux 發行版內建的 Qt 6.4 可用 `-DTV_QT_MIN=6.4`）與 OpenSSL ≥ 1.1.1（建議 3.x）。

## CI（結構沿用 Pomodoro repo）
`.github/workflows/`：

| 檔案 | 內容 |
|---|---|
| `windows.yml` | Windows_AMD64（原生）、Windows_ARM64（在 x64 上交叉編譯）。x64 以 windeployqt 附上 Qt 執行庫 |
| `linux.yml` | Linux_AMD64（ubuntu-22.04）、Linux_ARM64（ubuntu-24.04-arm，需 glibc ≥ 2.38） |
| `macos.yml` | Mac_Universal（arm64 + x86_64，OpenSSL 以 lipo 合併） |
| `release.yml` | 推送到 `main` 時呼叫上面三個；**五個全部成功**才發佈 Pre-release（Tag `CP#<次數>`，名稱 `CP#<日期時間>+<次數>`，UTC+8）。手動執行則只產生 `release-preview` artifact |
| `distro.yml` | Ubuntu / Debian / Fedora / Arch / openSUSE 以各自的 Qt + OpenSSL 編譯並測試（僅供參考，不擋發佈） |

`.github/scripts/package.py` 把各平台的 tar 轉成 `TokenVault-<平台>-CP<次數>.zip`（保留執行權限，附 `BUILD_INFO.txt`）。

| zip | 內容 |
|---|---|
| Windows_AMD64 | `TokenVault.exe` + Qt 執行庫，解壓即可執行 |
| Windows_ARM64 | 只有 `TokenVault.exe`，需自備 Qt 6.8.3 ARM64 執行庫 |
| Mac_Universal | `TokenVault.app`，需本機已安裝 Qt 6.8.3，未簽署 |
| Linux_AMD64 / Linux_ARM64 | 單一執行檔，需本機已安裝 Qt 6.8.3 執行庫 |

（這些「需自備 Qt」的限制與 Pomodoro 相同；首次 CI 全綠後再考慮加上 macdeployqt / AppImage 打包。）

設計理由與來源見 `DESIGN.md`。
