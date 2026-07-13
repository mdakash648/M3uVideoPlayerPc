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

### Step 0.1: CMake & Project Structure ⬜ Not Started
- [ ] Create `CMakeLists.txt` configured for C++17/20 and Qt6 (Core, Gui, Qml, Quick, Sql, Network).
- [ ] Set up folder structure: `src/ui`, `src/domain`, `src/data`, `src/infrastructure`.
- [ ] Configure `libmpv` linking in CMake.

### Step 0.2: Dependency Injection & App Init ⬜ Not Started
- [ ] Set up a basic IoC container or manual DI in `main.cpp`.
- [ ] Expose C++ ViewModels to QML via `qmlRegisterType` or `QML_ELEMENT`.

### Step 0.3: Global Theme & Design System (QML) ⬜ Not Started
- [ ] Define the dark color palette (`#121212` background, `#1E1E1E` surface, `#FFD54F` primary accent).
- [ ] Create reusable QML components (CustomButtons, TextStyles, ListDelegates).

---

## PHASE 1 — Data Layer & Playlist Management

### Step 1.1: Domain Models ⬜ Not Started
- [ ] Create `Playlist`, `Channel`, `Group`, `PlaybackHistory` structs/classes.
- [ ] Create enums for `ContentType` (LIVE, MOVIE, SERIES), `UpdateFrequency`, `TimeZoneMode`.

### Step 1.2: Database — SQLite via QtSql ⬜ Not Started
- [ ] Setup `QSqlDatabase` connection.
- [ ] Create tables: `Playlists`, `Channels`, `Groups`, `History`, `ResumePoints`.
- [ ] Implement CRUD operations using `QSqlQuery`.

### Step 1.3: M3U/M3U8 Parser & HTTP Referer Support ⬜ Not Started
- [ ] Create `M3uParser` in C++: Handle `#EXTINF`, `#EXTVLCOPT:http-referrer=`, user-agents, catch-up tags, duration.
- [ ] Support pipe-delimited stream URL headers (e.g., `url|Referer=...&User-Agent=...`) for servers responding with 403 Forbidden.

### Step 1.4: Xtream Codes API Client ⬜ Not Started
- [ ] Create `XtreamApiClient` using `QNetworkAccessManager`.
- [ ] Handle Auth, Live, VOD, and Series endpoints.

### Step 1.5: Repository & Use Cases ⬜ Not Started
- [ ] Implement `IPlaylistRepository`, `IChannelRepository`.
- [ ] Implement standard use cases, plus **DirectLinkUseCases** and **OpenExternalM3UUseCase**.

### Step 1.6: Trusted Time & TimeZone Manager ⬜ Not Started
- [ ] Implement `TrustedTimeSource` fetching network time (NTP or HTTP Date) independent of device clock.
- [ ] Implement `TimeZoneManager` (Auto vs Manual modes).
- [ ] Base playlist sync scheduler strictly on elapsed monotonic time + trusted network time to avoid sync bugs from incorrect device clocks.

---

## PHASE 2 — Core UI Screens (QML)

### Step 2.1: Main Window & Navigation Shell ⬜ Not Started
- [ ] Design `main.qml` with a sidebar and `StackView` for navigation.
- [ ] Handle on-start background sync.

### Step 2.2: Playlist & Direct Link Management ⬜ Not Started
- [ ] `PlaylistListView.qml`: Show user playlists with floating resume buttons.
- [ ] `DirectLinkView.qml`: Dedicated UI for pasting and playing a single direct stream URL.

### Step 2.3: Add/Edit Playlist Dialog ⬜ Not Started
- [ ] `AddPlaylistDialog.qml`: Support M3U (web/local), Xtream, Update Frequency, and Direct Links.

### Step 2.4: Group & Channel Browsing ⬜ Not Started
- [ ] `GroupListView.qml`: 3-column grid of folder tiles + Dual Global Search.
- [ ] `ChannelListView.qml`: 4 View Modes (List, Grid, Title Only, Poster), Sort logic.
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
