# M3U Video Player — Desktop Edition (C++ + Qt/QML + libmpv)

## 🎯 Full Implementation Plan & Progress Tracker

**Original Project:** `M3uVideoPlayer` (Android, Kotlin, MVVM + Clean Architecture, Room, Hilt, Media3/ExoPlayer)
**Target Platform:** Windows/Linux/macOS Desktop Application
**Tech Stack:** C++17/20 + Qt (QML) + libmpv + SQLite + CMake
**Architecture:** MVVM / Clean Architecture mapped to C++

**Status Legend:** ✅ Complete · 🔄 In Progress · ⬜ Not Started · ❌ Blocked

---

## Architecture Overview

```text
┌─────────────────────────────────────────────────────────────────────┐
│                        PRESENTATION LAYER (QML / Qt)                │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ │
│  │ Playlist │ │  Group   │ │ Channel  │ │  Player  │ │ Settings │ │
│  │  View    │ │  View    │ │  View    │ │  Window  │ │  View    │ │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘ │
│       │            │            │             │            │      │
│  ┌────┴─────┐ ┌────┴─────┐ ┌────┴─────┐ ┌────┴─────┐ ┌────┴─────┐ │
│  │ Playlist │ │  Group   │ │ Channel  │ │  Player  │ │ Settings │ │
│  │ViewModel │ │ ViewModel│ │ ViewModel│ │ ViewModel│ │ ViewModel│ │
│  │ (C++)    │ │  (C++)   │ │  (C++)   │ │  (C++)   │ │  (C++)   │ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│                         DOMAIN LAYER (C++)                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────────────┐ │
│  │   Models     │ │  Use Cases   │ │  Repository Interfaces       │ │
│  │ (Channel,    │ │ (AddPlaylist │ │  (IPlaylistRepo,             │ │
│  │  Playlist,   │ │  SyncPlaylist│ │   IChannelRepo,              │ │
│  │  Group, etc.)│ │  Search etc.)│ │   ISettingsRepo)             │ │
│  └──────────────┘ └──────────────┘ └──────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│                          DATA LAYER (C++)                           │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐ │
│  │  SQLite      │ │  M3U Parser  │ │  Xtream API  │ │ User Prefs │ │
│  │ (QtSql/C++)  │ │ + File Store │ │ (QNetwork)   │ │ (QSettings)│ │
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│                       INFRASTRUCTURE (C++)                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────────────┐ │
│  │    libmpv    │ │ Trusted Time │ │  Background Scheduler        │ │
│  │ (Media Engine│ │(NTP / Network│ │  (Timer-based auto-sync)     │ │
│  └──────────────┘ └──────────────┘ └──────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
```

---

## PHASE 0 — Project Setup & Foundation

### Step 0.1: CMake & Project Structure ✅ Complete
- [x] Create `CMakeLists.txt` configured for C++17/20 and Qt6 (Core, Gui, Qml, Quick, Sql, Network).
- [x] Set up folder structure: `src/ui`, `src/domain`, `src/data`, `src/infrastructure`.
- [x] Configure `libmpv` linking in CMake.

### Step 0.2: Dependency Injection & App Init 🔄 In Progress
- [x] Set up a basic IoC container or manual DI in `main.cpp`.
- [ ] Expose C++ ViewModels to QML via `qmlRegisterType` or `QML_ELEMENT`.

### Step 0.3: Global Theme & Design System (QML) 🔄 In Progress
- [x] Define the dark color palette (`#121212` background, `#1E1E1E` surface, `#FFD54F` primary accent).
- [x] Create reusable QML components (CustomButtons, TextStyles, ListDelegates).

---

## PHASE 1 — Data Layer & Playlist Management

### Step 1.1: Domain Models ✅ Complete
- [x] Create `Playlist`, `Channel`, `Group`, `PlaybackHistory` structs/classes.
- [x] Create enums for `ContentType` (LIVE, MOVIE, SERIES), `UpdateFrequency`, `TimeZoneMode`.

### Step 1.2: Database — SQLite via QtSql ✅ Complete
- [x] Setup `QSqlDatabase` connection.
- [x] Create tables: `Playlists`, `Channels`, `Groups`, `History`, `ResumePoints`.
- [x] Implement CRUD operations using `QSqlQuery`.

### Step 1.3: M3U/M3U8 Parser & HTTP Referer Support ✅ Complete
- [x] Create `M3uParser` in C++: Handle `#EXTINF`, `#EXTVLCOPT:http-referrer=`, user-agents, catch-up tags, duration.
- [x] Support pipe-delimited stream URL headers (e.g., `url|Referer=...&User-Agent=...`) for servers responding with 403 Forbidden.

### Step 1.4: Xtream Codes API Client ✅ Complete
- [x] Create `XtreamApiClient` using `QNetworkAccessManager`.
- [x] Handle Auth, Live, VOD, and Series endpoints.

### Step 1.5: Repository & Use Cases ✅ Complete
- [x] Implement `IPlaylistRepository`, `IChannelRepository`.
- [x] Implement standard use cases, plus **DirectLinkUseCases** and **OpenExternalM3UUseCase**.

### Step 1.6: Trusted Time & TimeZone Manager ✅ Complete
- [x] Implement `TrustedTimeSource` fetching network time (NTP or HTTP Date) independent of device clock.
- [x] Implement `TimeZoneManager` (Auto vs Manual modes).
- [x] Base playlist sync scheduler strictly on elapsed monotonic time + trusted network time to avoid sync bugs from incorrect device clocks.

---

## PHASE 2 — Core UI Screens (QML)

### Step 2.1: Main Window & Navigation Shell 🔄 In Progress
- [x] Design `main.qml` with a sidebar and `StackView` for navigation.
- [ ] Handle on-start background sync.

### Step 2.2: Playlist & Direct Link Management ✅ Complete
- [x] `PlaylistListView.qml`: Show user playlists with floating resume buttons.
- [x] `DirectLinkView.qml`: Dedicated UI for pasting and playing a single direct stream URL.

### Step 2.3: Add/Edit Playlist Dialog ✅ Complete
- [x] `AddPlaylistDialog.qml`: Support M3U (web/local), Xtream, Update Frequency, and Direct Links.

### Step 2.4: Group & Channel Browsing 🔄 In Progress
- [x] `GroupListView.qml`: 3-column grid of folder tiles + Dual Global Search.
- [x] `ChannelListView.qml`: 4 View Modes (List, Grid, Title Only, Poster), Sort logic.
- [ ] Fuzzy Search Engine implemented in C++.

---

## PHASE 3 — Video Player (libmpv + QML)

### Step 3.1: Video Playback Engine (libmpv) ⬜ Not Started
- [ ] Integrate `libmpv` with Qt/QML (using `mpv_render_context` or `QQuickFramebufferObject`).
- [ ] Implement `PlaybackEngine` abstraction allowing potential future engines, but defaulting to `MpvPlaybackEngine`.
- [ ] Pass dynamic HTTP headers (Referer, User-Agent) to mpv properties.
- [ ] Ensure hardware acceleration is enabled (`hwdec=auto`).

### Step 3.2: Audio Codec Support & Quality Selection ⬜ Not Started
- [ ] Ensure `libmpv` natively decodes DD+/Atmos/E-AC3 audio (solves the ExoPlayer Android TV bug).
- [ ] Implement `QualityUrlParser` to read multi-bitrate HLS/DASH streams.
- [ ] Add Quality selector in Player OSD.

### Step 3.3: Player Controls & OSD ⬜ Not Started
- [ ] QML Player Overlay: Auto-hiding top/bottom bars.
- [ ] VLC-style OSD row: Subtitles, Audio Tracks, Playback Speed (0.5x-2.0x), Quality, PiP.
- [ ] Next/Previous Queue navigation loop.

### Step 3.4: Mouse, Gestures, & Keyboard Controls ⬜ Not Started
- [ ] Accumulative Seek: Double click left/right for ±10s.
- [ ] Vertical Drag: Left side = Brightness, Right side = Volume (0-200% software gain).
- [ ] Keyboard: Space (Play/Pause), Arrows (Seek/Volume), `M` (Mute), `F` (Fullscreen).

### Step 3.5: Resume Points & Catch-up ⬜ Not Started
- [ ] Save resume points every 5s for VODs. Mark completed if > 95%.
- [ ] Catch-up playback: Resolve archive URLs and play past broadcasts.

---

## PHASE 4 — Settings & Utilities

### Step 4.1: Settings Screen ⬜ Not Started
- [ ] Manage Playlists.
- [ ] TimeZone settings: Toggle Auto / Manual Timezone selection with Timezone Picker Dialog.
- [ ] Engine settings (if abstracted).

### Step 4.2: Backup & Restore ⬜ Not Started
- [ ] Export/Import SQLite DB + QSettings to a file.

### Step 4.3: Playlist Auto-Sync Scheduler ⬜ Not Started
- [ ] QTimer-based background sync using `TrustedTimeSource`.

### Step 4.4: History & Favorites ⬜ Not Started
- [ ] Global Favorites screen and Playback History lists.

---

## PHASE 5 — Polish & Advanced Features

### Step 5.1: Channel Logo Loading & Caching ⬜ Not Started
- [ ] C++ network image provider or standard QML `Image` with disk caching.

### Step 5.2: Always-on-Top Mode (PiP) ⬜ Not Started
- [ ] Floating, topmost frameless window mode.

### Step 5.3: Application Packaging ⬜ Not Started
- [ ] CPack / CMake install rules for Windows/Linux/macOS deployment.

---

## PHASE 6 — Testing & Verification
- [ ] Unit Tests (Parser, Quality Selector, Trusted Time).
- [ ] Manual UI Verification.
