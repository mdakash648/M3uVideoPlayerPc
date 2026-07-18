# M3U Video Player PC

A modern, fast, and feature-rich M3U/IPTV video player for Windows, built with **C++**, **Qt 6 (QML)**, and powered by the robust **libmpv** media engine.

## ✨ Features

### 📺 Advanced Playback Engine
* **libmpv Integration:** Hardware-accelerated decoding with support for almost any video format (HLS, TS, MP4, MKV) and up to 4K resolutions.
* **Smart Resume:** Automatically remembers your exact playback position for Movies and TV Series so you can resume where you left off.
* **Multi-Track Support:** Easily switch between multiple **Audio Tracks** and **Subtitles** embedded in the video stream.
* **Auto Quality Fallback:** Automatically switches between available stream qualities (1080p, 720p, 480p) if a stream fails to load.

### 📋 M3U / IPTV Support
* **Playlist Management:** Add, edit, and organize multiple remote or local M3U/M3U8 playlists.
* **Live TV Logos:** Automatically parses and displays embedded `tvg-logo` images for IPTV channels.
* **Fuzzy Search:** Blazing fast, typo-tolerant search to instantly find channels or movies within massive playlists.
* **Auto Updates:** Background synchronization to keep remote playlists up to date based on your preferred schedule.

### 🎬 Movie & Series Enhancements
* **Auto Movie Posters (OMDb API):** Automatically fetches high-quality, professional posters for your VOD (Video on Demand) content like Movies and Series.
* **Smart Categorization:** Automatically detects and categorizes content into Live TV, Movies, or Series based on naming patterns.

### ⚡ Direct Stream & Clipboard
* **Direct Play:** Paste a direct video link or M3U file to play immediately without saving a full playlist.
* **Clipboard Auto-Paste:** Seamlessly detects stream URLs from your clipboard and automatically pastes them into the player for instant viewing.
* **History Log:** Automatically keeps track of all your manually played streams and direct links in a dedicated "History" playlist.

### 🎨 Modern User Interface
* **QML UI:** A sleek, fully responsive dark-mode interface with smooth animations and hover effects.
* **Customizable Views:** Switch between Grid and List views, with customizable column counts for optimal browsing.
* **Keyboard Shortcuts:** Full keyboard navigation for media controls (Space to pause/play, Arrows to seek).

### 💾 Backup & Restore
* **Export Data:** Easily backup your entire app state (including all playlists, groups, history, and settings) to a single JSON file.
* **Restore:** Load your configurations instantly on a new machine.

## 🛠️ Technology Stack
* **Language:** C++20
* **Framework:** Qt 6.11 (Core, Gui, Qml, Quick, QuickControls2, Sql, Network, Concurrent)
* **Media Engine:** libmpv
* **Build System:** CMake
* **OS Support:** Windows (MinGW 64-bit)

## 📦 Building for Windows

To package the application as a standalone `.exe`:
1. Open the project in Qt Creator or use CMake.
2. Build the project in **Release Mode**.
3. A `M3uVideoPlayerPc_Release` folder will be generated containing the `.exe` along with all necessary Qt and mpv `.dll` files.
4. Run `installer.iss` using **Inno Setup** to compile a professional, distributable Windows Installer (`.exe`).
